/*
 * Copyright (c) 2017 Sound <sound@sagaforce.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "SSHDataStore.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <memory>
#include <type_traits>
#include "libnebula/Exception.h"
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include "libnebula/InputStream.h"
#include "libnebula/OutputStream.h"

namespace Nebula
{
	SSHDataStore::SSHDataStore(const char *path)
	: mPath(path)
	{
		mSession = ssh_new();
		if (!mSession) {
			throw InitializationException("Failed to initialize SSH.");
		}
	}
	
	SSHDataStore::~SSHDataStore()
	{
		sftp_free(mFtp);
		ssh_disconnect(mSession);
		ssh_free(mSession);
	}
	
	bool SSHDataStore::connect(const char *hostname, int port, const char *username, Authentication& auth)
	{
		ssh_options_set(mSession, SSH_OPTIONS_HOST, hostname);
		ssh_options_set(mSession, SSH_OPTIONS_PORT, &port);
		
		int status;
		while((status = ssh_connect(mSession)) == SSH_AGAIN) {
			
		}
		
		if(status != SSH_OK) {
			throw ConnectionException("Failed to open connection.");
		}
		
		unsigned char *hash;
		int state = ssh_is_server_known(mSession);
		
		int hlen = ssh_get_pubkey_hash(mSession, &hash);
		std::unique_ptr<unsigned char, decltype(free) *> hashPtr { hash, free };
		if (hlen < 0) {
			throw ConnectionException("Failed to get SSH key hash.");
		}
		
		switch (state)
		{
			case SSH_SERVER_KNOWN_OK:
				break; /* ok */
			case SSH_SERVER_KNOWN_CHANGED:
				throw ConnectionException("Host key for server changed");
				break;
			case SSH_SERVER_FOUND_OTHER:
				throw ConnectionException("The host key for this server was not found but an other type of key exists.");
				break;
			case SSH_SERVER_FILE_NOT_FOUND:
				/* fallback to SSH_SERVER_NOT_KNOWN behavior */
			case SSH_SERVER_NOT_KNOWN:
				if(auth.acceptHostKey(hash, hlen)) {
					if (ssh_write_knownhost(mSession) < 0)
					{
						throw ConnectionException(strerror(errno));
					}
					
				} else {
					throw ConnectionException("Host key was not accepted.");
				}
				break;
			case SSH_SERVER_ERROR:
				throw ConnectionException(ssh_get_error(mSession));
				break;
		}
		
		if(!auth.authenticate()) {
			return false;
		}
		
		mFtp = sftp_new(mSession);
		if(!mFtp) {
			throw ConnectionException("Failed to create SFTP.");
		}
		
		return true;
	}
	
	bool SSHDataStore::PasswordAuthentication::authenticate()
	{
		int authStatus;
		while((authStatus = ssh_userauth_password(mSession, mUsername.c_str(), mPassword.c_str())) == SSH_AUTH_AGAIN) { }
		switch(authStatus) {
			case SSH_AUTH_ERROR:
			case SSH_AUTH_PARTIAL:
				throw ConnectionException("Authentication failed with username and password.");
			case SSH_AUTH_DENIED:
				return false;
		}

		return true;
	}
	
	SSHDataStore::PublicKeyAuthentication::PublicKeyAuthentication(ssh_session session, ZeroedString username, ZeroedString password, const char *privateKeyFile)
	: Authentication(session)
	, mPrivateKeyFile(privateKeyFile)
	, mUsername(username)
	, mPassword(password)
	{
	}
	
	SSHDataStore::PublicKeyAuthentication::~PublicKeyAuthentication()
	{
	}
	
	bool SSHDataStore::PublicKeyAuthentication::authenticate()
	{
		ssh_key sshKey;
		ssh_pki_import_privkey_file(mPrivateKeyFile.c_str(), mPassword.c_str(), nullptr, nullptr, &sshKey);
		std::unique_ptr<std::remove_pointer<ssh_key>::type, decltype(ssh_key_free) *> sshKeyPtr { sshKey, ssh_key_free };

		int authStatus;
		authStatus = ssh_userauth_publickey(mSession, mUsername.c_str(), sshKey);
		switch(authStatus) {
			case SSH_AUTH_ERROR:
			case SSH_AUTH_PARTIAL:
				throw ConnectionException("Authentication failed with username and password.");
			case SSH_AUTH_DENIED:
				return false;
		}
		
		return true;
	}
	
	bool SSHDataStore::exist(const char *path, ProgressFunction progress)
	{
		sftp_attributes attr = sftp_lstat (mFtp, (mPath / path).c_str());
		if(!attr) return false;
		sftp_attributes_free(attr);
		return true;
	}
	
	void SSHDataStore::get(const char *path, OutputStream& stream, ProgressFunction progress)
	{
		std::unique_ptr<std::remove_pointer<sftp_file>::type, decltype(sftp_close) *>
			fp { sftp_open(mFtp, (mPath / path).c_str(), O_RDONLY, 0), sftp_close };
		if(!fp) {
			throw FileNotFoundException("File not found.");
		}
		
		uint8_t buffer[8192];
		ssize_t n, count = 0;
		while((n = sftp_read(fp.get(), buffer, sizeof(buffer))) > 0) {
			count += n;
			if(!progress(count, -1)) {
				throw CancelledException("User cancelled.");
			}
			stream.write(buffer, n);
		}
	}
	
	void SSHDataStore::put(const char *path, InputStream& stream, ProgressFunction progress)
	{
		std::unique_ptr<std::remove_pointer<sftp_file>::type, decltype(sftp_close) *>
			fp { sftp_open(mFtp, (mPath / path).c_str(), O_WRONLY, 0600), sftp_close };
		if(!fp) {
			throw FileNotFoundException("File not found.");
		}
		
		uint8_t buffer[8192];
		ssize_t n, count = 0;
		while((n = stream.read(buffer, sizeof(buffer))) > 0) {
			count += n;
			if(!progress(count, -1)) {
				throw CancelledException("User cancelled.");
			}
			sftp_write(fp.get(), buffer, n);
		}
	}
	
	void SSHDataStore::list(const char *path, std::function<void (const char *, void *)> listCallback, void *userData, ProgressFunction progress)
	{
		std::unique_ptr<std::remove_pointer<sftp_dir>::type, decltype(sftp_closedir) *>
			dir { sftp_opendir(mFtp, (mPath / path).c_str()), sftp_closedir };
		if(!dir) {
			throw FileNotFoundException("File not found.");
		}
		
		int i = 0;
		sftp_attributes attributes;
		while((attributes = sftp_readdir(mFtp, dir.get())) != nullptr)
		{
			if(!progress(i++, dir->count)) {
				throw CancelledException("User cancelled.");
			}
			listCallback(attributes->name, userData);
			sftp_attributes_free(attributes);
		}
	}
	
	bool SSHDataStore::unlink(const char *path, ProgressFunction progress)
	{
		return sftp_unlink(mFtp, (mPath / path).c_str()) == 0;
	}
}
