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

#pragma once

#include <boost/filesystem.hpp>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include "ZeroedString.h"
#include "DataStore.h"

namespace Nebula
{
	class SSHDataStore : public DataStore
	{
	public:
		SSHDataStore(const char *path);
		~SSHDataStore();
		
		class Authentication
		{
		public:
			explicit Authentication(ssh_session session) : mSession(session) {}
			virtual ~Authentication() { }
			
			bool (*acceptHostKey)(const uint8_t *hostKey, int len);
			
			virtual bool authenticate() = 0;
		protected:
			ssh_session mSession;
		};
		
		class PasswordAuthentication : public Authentication
		{
		public:
			explicit PasswordAuthentication(ssh_session session, ZeroedString username, ZeroedString password)
			: Authentication(session)
			, mUsername(username)
			, mPassword(password)
			{
			}
			
			virtual bool authenticate() override;
		private:
			ZeroedString mUsername;
			ZeroedString mPassword;
		};
		
		class PublicKeyAuthentication : public Authentication
		{
		public:
			PublicKeyAuthentication(ssh_session session, ZeroedString username, ZeroedString password, const char *privateKeyFile);
			~PublicKeyAuthentication();
			
			virtual bool authenticate() override;
		private:
			std::string mPrivateKeyFile;
			ZeroedString mUsername;
			ZeroedString mPassword;
		};
		
		bool connect(const char *hostname, int port, const char *username, Authentication& auth);

		virtual bool exist(const char *path, ProgressFunction progress = DefaultProgressFunction) override;
		virtual void get(const char *path, OutputStream& stream, ProgressFunction progress = DefaultProgressFunction) override;
		virtual void put(const char *path, InputStream& stream, ProgressFunction progress = DefaultProgressFunction) override;
		virtual void list(const char *path, std::function<void (const char *, void *)> listCallback, void *userData = nullptr, ProgressFunction progress = DefaultProgressFunction) override;
		virtual bool unlink(const char *path, ProgressFunction progress = DefaultProgressFunction) override;
	private:
		boost::filesystem::path mPath;
		ssh_session mSession;
		sftp_session mFtp;
		
		void initializeConnection(const char *hostname, int port, bool (*acceptHostKey)(const uint8_t *hostKey, int len));
		void initializeSFTP();
	};
}
