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
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#ifdef _WIN32
#else
#include <termios.h>
#include <unistd.h>
#endif

#include <memory>
#include <functional>
#include <boost/filesystem.hpp>
#include "libnebula/DataStore.h"
#include "libnebula/backends/FileDataStore.h"
#include "libnebula/backends/SshDataStore.h"
#include "libnebula/FileStream.h"
#include "libnebula/Exception.h"
#include "libnebula/FileInfo.h"
#include "libnebula/Repository.h"

static void printHelp()
{
	printf("usage: NebulaBackup [options] init <repo>\n");
	printf("       NebulaBackup [options] backup <repo> <snapshot> <file> [<file>...]\n");
	printf("       NebulaBackup [options] restore <repo> <snapshot> <destdir>\n");
	printf("       NebulaBackup [options] download <repo> <snapshot> <file> [<file>...]\n");
	printf("       NebulaBackup [options] list <repo>\n");
	printf("       NebulaBackup [options] list <repo> <snapshot>\n");
	printf("       NebulaBackup [options] delete <repo> <snapshot>\n");
	printf("       NebulaBackup [options] compact <repo>\n");
	printf("       NebulaBackup [options] password <repo>\n");
	printf("\n");
	printf("<repo> can be in the format of:\n");
	printf("       [backend://][user@][hostname:]/path\n");
	printf("\n");
	printf("options:\n");
	printf(" -q, --quiet              Run without output\n");
	printf(" -b, --backend            Explicitly specify the backend\n");
	printf("\n");
	printf("ssh backend options:\n");
	printf(" -u, --username=USER      SSH username\n");
	printf(" -p, --password=PASSWORD  SSH password\n");
	printf("\n");
	printf("s3 backend options:\n");
	printf("     --aws-bucket=BUCKET  AWS bucket\n");
	printf("     --aws-token=TOKEN    AWS token\n");
	printf("\n");
}

struct Options
{
	int quiet;
	
	Options() : quiet(0) { }
};

static Options options;

static std::shared_ptr<Nebula::DataStore> createDataStoreFromRepository(const char *repo)
{
	using namespace Nebula;
	using namespace boost;

#if 0
	// guess the format
	const char sshProto[] = "ssh://";
	if(strncmp(repo, sshProto, sizeof(sshProto) - 1) == 0) {
		
	}
	//
#endif

	filesystem::path path = repo;
	
	if(!filesystem::exists(path)) {
		throw InvalidRepositoryException("Repository not found at path.");
	}

	return std::make_shared<FileDataStore>(path);
}

static void enableConsoleOutput(bool enable)
{
#ifdef _WIN32
#else
	struct termios tty;
	tcgetattr(STDIN_FILENO, &tty);

	if(enable) {
		tty.c_lflag |= ECHO;
	} else {
		tty.c_lflag &= ~ECHO;
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
}

Nebula::ZeroedString promptReadPassword(bool confirm)
{
	using namespace Nebula;

	char passwd1[64], passwd2[64];
	ZeroedString outPassword;
	
	enableConsoleOutput(false);
	
	std::unique_ptr<void, std::function<void (void *)>>
		onExit(passwd1, [&passwd1, &passwd2](void *) {
		explicit_bzero(passwd1, sizeof(passwd1));
		explicit_bzero(passwd2, sizeof(passwd2));
		enableConsoleOutput(true);
	});
	
	printf("Password: ");
	if(!fgets(passwd1, sizeof(passwd1), stdin)) {
		throw InvalidDataException("Password string is too long.");
	}
	printf("\n");
	
	if(confirm) {
		printf("Confirm password: ");
		if(!fgets(passwd2, sizeof(passwd2), stdin)) {
			throw InvalidDataException("Password string is too long.");
		}
		printf("\n");

		if(strncmp(passwd1, passwd2, sizeof(passwd1)) != 0) {
			throw InvalidDataException("Passwords don't match.");
		}
	}

	// trim the new line
	size_t n = strnlen(passwd1, sizeof(passwd1));
	while (passwd1[--n] == '\n') {
		passwd1[n] = 0;
	}

	outPassword = passwd1;

	return outPassword;
}

static void initializeRepository(const char *repository)
{
	using namespace Nebula;

	auto dataStore = createDataStoreFromRepository(repository);
	Repository repo(dataStore.get());

	ZeroedString password = promptReadPassword(true);
	repo.initializeRepository(password.c_str());
	
	if(!options.quiet) {
		printf("Repository initialized.\n");
	}
}

static void backupFiles(const char *repository, const char *snapshotName, int argc, char * const *argv)
{
	using namespace Nebula;
	using namespace boost;
	
	auto dataStore = createDataStoreFromRepository(repository);
	Repository repo(dataStore.get());
	
	ZeroedString password = promptReadPassword(false);
	if(!repo.unlockRepository(password.c_str())) {
		throw RepositoryException("Unable to unlock repository. Password was incorrect.");
	}

	std::unique_ptr<Snapshot> snapshot(repo.createSnapshot());
	for(int i = 0; i < argc; ++i) {
		if(filesystem::is_directory(argv[i])) {
			filesystem::recursive_directory_iterator dirIterator(argv[i]);
			
			for(auto& file : dirIterator)
			{
				if(!filesystem::is_directory(file))
				{
					if(!options.quiet) {
						printf("%s\n", file.path().c_str());
					}
					FileStream fs(file.path().c_str(), FileMode::Read);
					repo.uploadFile(snapshot.get(), file.path().c_str(), fs);
				}
			}
		} else {
			if(!options.quiet) {
				printf("%s\n", argv[i]);
			}
			FileStream fs(argv[i], FileMode::Read);
			repo.uploadFile(snapshot.get(), argv[i], fs);
		}
	}
	
	repo.commitSnapshot(snapshot.get(), snapshotName);
}

static void listSnapshots(const char *repository)
{
	using namespace Nebula;
	using namespace boost;
	
	auto dataStore = createDataStoreFromRepository(repository);
	Repository repo(dataStore.get());
	
	ZeroedString password = promptReadPassword(false);
	if(!repo.unlockRepository(password.c_str())) {
		throw RepositoryException("Unable to unlock repository. Password was incorrect.");
	}
	
	repo.listSnapshots([](const char *snapshotName) {
		if(snapshotName) {
			printf("%s\n", snapshotName);	
		}
	});
}

static void listSnapshot(const char *repository, const char *snapshotName)
{
	using namespace Nebula;
	using namespace boost;
	
	auto dataStore = createDataStoreFromRepository(repository);
	Repository repo(dataStore.get());
	
	ZeroedString password = promptReadPassword(false);
	if(!repo.unlockRepository(password.c_str())) {
		throw RepositoryException("Unable to unlock repository. Password was incorrect.");
	}
	
	std::unique_ptr<Snapshot> snapshot(repo.loadSnapshot(snapshotName));
	snapshot->forEachFileEntry([&snapshot] (const Snapshot::FileEntry &fe) {
		const char *user = snapshot->indexToString(fe.userIndex);
		const char *group = snapshot->indexToString(fe.groupIndex);
		const char *path = snapshot->indexToString(fe.pathIndex);

		char modeString[] = "----------";
		switch((FileType)fe.type) {
			case FileType::Directory: modeString[0] = 'd'; break;
			case FileType::SymbolicLink: modeString[0] = 'l'; break;
			case FileType::BlockDevice: modeString[0] = 'b'; break;
			case FileType::CharacterDevice: modeString[0] = 'c'; break;
			default: break;
		}
		if(fe.mode & 0400) modeString[1] = 'r';
		if(fe.mode & 0200) modeString[2] = 'w';
		if(fe.mode & 0100) modeString[3] = 'x';
		if(fe.mode & 0040) modeString[4] = 'r';
		if(fe.mode & 0020) modeString[5] = 'w';
		if(fe.mode & 0010) modeString[6] = 'x';
		if(fe.mode & 0004) modeString[7] = 'r';
		if(fe.mode & 0002) modeString[8] = 'w';
		if(fe.mode & 0001) modeString[9] = 'x';
		
		if(fe.mode & 04000) modeString[3] = (fe.mode & 0100) ? 's' : 'S';
		if(fe.mode & 02000) modeString[6] = (fe.mode & 0010) ? 's' : 'S';
		if(fe.mode & 01000) modeString[9] = (fe.mode & 0001) ? 't' : 'T';
		
		printf("%-11s %-8s %-8s %10llu %s\n",
			   modeString,
			   user,
			   group,
			   fe.size,
			   path);
	});
}

static void downloadFiles(const char *repository, const char *snapshotName, int argc, char *argv[])
{
	using namespace Nebula;
	using namespace boost;
	
	auto dataStore = createDataStoreFromRepository(repository);
	Repository repo(dataStore.get());
	
	ZeroedString password = promptReadPassword(false);
	if(!repo.unlockRepository(password.c_str())) {
		throw RepositoryException("Invalid password.");
	}
	
	std::unique_ptr<Snapshot> snapshot(repo.loadSnapshot(snapshotName));
	for(int i = 0; i < argc; ++i) {
		filesystem::path destPath = filesystem::path(argv[i]).filename();
		
		if(filesystem::exists(destPath)) {
			printf("%s: File exists. Overwrite (y/n)? ", destPath.c_str());
			fflush(stdout);
			if(fgetc(stdin) != 'y') {
				continue;
			}
		}
		
		if(!options.quiet) {
			printf("%s\n", destPath.c_str());
		}
		FileStream outStream(destPath.c_str(), FileMode::Write);
		repo.downloadFile(snapshot.get(), argv[i], outStream);
	}
}

static void changePassword(const char *repository)
{
	using namespace Nebula;
	using namespace boost;
	
	auto dataStore = createDataStoreFromRepository(repository);
	Repository repo(dataStore.get());
	
	printf("Old password\n");
	ZeroedString oldPassword = promptReadPassword(false);
	
	if(!repo.unlockRepository(oldPassword.c_str())) {
		throw RepositoryException("Invalid password.");
	}
	
	printf("New password\n");
	ZeroedString newPassword = promptReadPassword(true);
	repo.changePassword(oldPassword.c_str(), newPassword.c_str());
	
	printf("Password updated.\n");
}

int main(int argc, char *argv[])
{
	using namespace Nebula;

	static struct option longOptions[] =
	{
		{ "help", no_argument, 0, 0 },
		{ "quiet", no_argument, 0, 'q' },
		{ 0, 0, 0, 0 }
	};
	
	int c;
	int optIndex;
	while((c = getopt_long(argc, argv, "q", longOptions, &optIndex)) >= 0) {
		switch (c) {
			case 0:
				
				break;
			case 'q':
				options.quiet = 0;
				break;
			default:
				printHelp();
				return -1;
		}
	}
	
	if (optind + 2 > argc) {
		printHelp();
		return -1;
	}

	try {
		const char *action = argv[optind];
		const char *repo = argv[optind + 1];
		switch(action[0]) {
				
			case 'i':
			{
				if(strcmp(action, "init") != 0) {
					throw InvalidArgumentException(std::string("Invalid action: ") + action);
				}
				
				initializeRepository(repo);
				break;
			}
				
			case 'b':
				if(strcmp(action, "backup") != 0) {
					throw InvalidArgumentException(std::string("Invalid action: ") + action);
				}
				
				if(optind + 3 > argc) {
					throw InvalidArgumentException("A snapshot name must be specified.");
				}
				
				if(optind + 4 > argc) {
					throw InvalidArgumentException("Atleast 1 file must be specified.");
				}
				
				backupFiles(repo, argv[optind + 2], argc - (optind + 3), argv + optind + 3);
				break;
			case 'd':
				if(strcmp(action, "download") == 0) {
					if(optind + 3 > argc) {
						throw InvalidArgumentException("A snapshot name must be specified.");
					}
					
					if(optind + 4 > argc) {
						throw InvalidArgumentException("Atleast 1 file must be specified.");
					}
					
					downloadFiles(repo, argv[optind + 2], argc - (optind + 3), argv + optind + 3);
				} else {
					throw InvalidArgumentException(std::string("Invalid action: ") + action);
				}
				break;
			case 'l':
				if(strcmp(action, "list") != 0) {
					throw InvalidArgumentException(std::string("Invalid action: ") + action);
				}
				
				if(optind + 3 > argc) {
					listSnapshots(repo);
				} else {
					listSnapshot(repo, argv[optind + 2]);
				}
				break;
			case 'p':
				if(strcmp(action, "password") != 0) {
					throw InvalidArgumentException(std::string("Invalid action: ") + action);
				}
				
				changePassword(repo);
				break;
		}
	} catch(const std::exception& e) {
		fprintf(stderr, "error: %s\n", e.what());
	}

	return 0;
}
