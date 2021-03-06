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
#include <time.h>
#include <signal.h>

#ifdef _WIN32
#else
#include <termios.h>
#include <unistd.h>
#endif

#include <memory>
#include <functional>
#include <strstream>
#include <boost/filesystem.hpp>
#include <openssl/evp.h>
#include "libnebula/DataStore.h"
#include "libnebula/backends/FileDataStore.h"
#include "libnebula/backends/SshDataStore.h"
#include "libnebula/backends/AwsS3DataStore.h"
#include "libnebula/FileStream.h"
#include "libnebula/ZeroedArray.h"
#include "libnebula/Exception.h"
#include "libnebula/FileInfo.h"
#include "libnebula/Repository.h"

static void printHelp()
{
	printf("usage: NebulaBackup [options] init <repo>\n");
	printf("       NebulaBackup [options] backup <repo> <snapshot> <file> [<file>...]\n");
	printf("       NebulaBackup [options] restore <repo> <snapshot> [<file>...] <destdir>\n");
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
	printf("     --no-verify          Don't downloaded files\n");
	printf(" -n, --dry-run            Dry-run\n");
	printf(" -f, --force              Don't prompt for overwrite\n");
	printf("\n");
	printf("ssh backend options:\n");
	printf(" -u, --username=USER      SSH username\n");
	printf(" -p, --password=PASSWORD  SSH password\n");
	printf("\n");
	printf("s3 backend environment variables and options:\n");
	printf("     <repo>               AWS bucket name\n");
	printf("     AWS_ACCESS_KEY       AWS access key\n");
	printf("     AWS_SECRET_KEY       AWS secret key\n");
	printf("\n");
}

static volatile bool sUserCancelled = false;
static void sigHandler(int code)
{
	sUserCancelled = true;
}

struct Options
{
	bool quiet;
	bool verify;
	bool dryRun;
	bool force;
	std::string backend;

	Options()
	: quiet(false)
	, verify(true)
	, dryRun(false)
	, force(false) { }
};

static Options options;

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

static Nebula::ZeroedString promptReadPassword(bool confirm)
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

static Nebula::ZeroedString promptInput(const char *prompt, bool isPassword = false)
{
	using namespace Nebula;

	ZeroedArray<char, 64> inputString;
	
	if(isPassword) {
		enableConsoleOutput(false);
	}
	
	std::unique_ptr<void, std::function<void (void *)>>
	onExit(&inputString, [isPassword](void *) {
		if(isPassword) {
			enableConsoleOutput(true);
		}
	});
	
	printf("%s: ", prompt);
	if(!fgets(inputString.data(), inputString.size(), stdin)) {
		throw InvalidDataException("Input is too long.");
	}
	printf("\n");

	// trim the new line
	size_t n = strnlen(inputString.data(), inputString.size());
	while (inputString[--n] == '\n') {
		inputString[n] = 0;
	}
	
	return inputString.data();
}

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

	if(options.backend == "s3") {
		ZeroedString bucket = repo;
		ZeroedString accessKey = getenv("AWS_ACCESS_KEY");
		ZeroedString secretKey = getenv("AWS_SECRET_KEY");
		
		if(accessKey.empty()) accessKey = promptInput("Aws Access Key: ", true);
		if(secretKey.empty()) secretKey = promptInput("Aws Secret Key: ", true);

		return std::make_shared<AwsS3DataStore>(bucket.data(), accessKey, secretKey);
	} else {
		filesystem::path path = repo;
		
		if(!filesystem::exists(path)) {
			throw InvalidRepositoryException("Repository not found at path.");
		}
		
		return std::make_shared<FileDataStore>(path);
	}

	return nullptr;
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


static void formatBytes(long bytes, char *outString, size_t maxLen)
{
	if(bytes < 1000L) {
		snprintf(outString, maxLen, "%ld b", bytes);
	} else if(bytes < 1000000L) {
		snprintf(outString, maxLen, "%.2f KB", (double)bytes / 1000.0);
	} else if(bytes < 1000000000L) {
		snprintf(outString, maxLen, "%.2f MB", (double)bytes / 1000000.0);
	} else if(bytes < 1000000000000L) {
		snprintf(outString, maxLen, "%.2f GB", (double)bytes / 1000000000.0);
	} else {
		snprintf(outString, maxLen, "%.2f TB", (double)bytes / 1000000000000.0);
	}
}

static void printProgress(int blockNo, int blockMax, long bytesTransferred, long bytesTotal, time_t startTime)
{
	if(options.quiet) return;

	long percent = bytesTotal > 0 ? bytesTransferred * 100 / bytesTotal : 100;
	fputs("  ", stdout);
	
	if(blockMax < 0) {
		fprintf(stdout, "# %7d", blockNo + 1);
	} else {
		fprintf(stdout, "# %3d/%-3d", blockNo + 1, blockMax);
	}

	fputs(" [", stdout);
	int i = 0;
	for(i = 0; i < percent; i += 5) {
		fputc('#', stdout);
	}
	for(; i < 100; i += 5) {
		fputc(' ', stdout);
	}
	
	char bytesTransferredString[32];
	char bytesTotalString[32];
	char avgBytesPerSecString[32];
	
	formatBytes(bytesTransferred, bytesTransferredString, sizeof(bytesTransferredString));
	formatBytes(bytesTotal, bytesTotalString, sizeof(bytesTotalString));
	
	time_t timeDelta = time(nullptr) - startTime;
	long avgBytesPerSec = timeDelta > 0 ? bytesTransferred / timeDelta : 0;
	formatBytes(avgBytesPerSec, avgBytesPerSecString, sizeof(avgBytesPerSecString));
	
	printf("] %s / %s  (%s/s)          \r", bytesTransferredString, bytesTotalString, avgBytesPerSecString);
	fflush(stdout);
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

	std::shared_ptr<Snapshot> snapshot(repo.createSnapshot());
	
	std::shared_ptr<Repository::PackUploadState> packState = repo.createPackState();
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
					
					if(!options.dryRun) {
						time_t startTime = time(nullptr);
						FileStream fs(file.path().c_str(), FileMode::Read);
						int lastBlockNo = 0;
						repo.uploadFile(packState, snapshot, file.path().c_str(), fs,
							[&lastBlockNo, startTime](int blockNo, int blockMax, long bytesUploaded, long bytesTotal) -> bool {
								if(lastBlockNo != blockNo) {
									lastBlockNo = blockNo;
									fputs("\n", stdout);
								}
								printProgress(blockNo, blockMax, bytesUploaded, bytesTotal, startTime);
								return !sUserCancelled;
							});
						if(!options.quiet) fputs("\n", stdout);
					}
				}
			}
		} else {
			if(!options.quiet) {
				printf("%s\n", argv[i]);
			}
			if(!options.dryRun) {
				time_t startTime = time(nullptr);
				FileStream fs(argv[i], FileMode::Read);
				int lastBlockNo = 0;
				repo.uploadFile(packState, snapshot, argv[i], fs,
								[&lastBlockNo, startTime](int blockNo, int blockMax, long bytesDownloaded, long bytesTotal) -> bool {
									if(lastBlockNo != blockNo) {
										lastBlockNo = blockNo;
										fputs("\n", stdout);
									}
									printProgress(blockNo, blockMax, bytesDownloaded, bytesTotal, startTime);
									return !sUserCancelled;
								});
				if(!options.quiet) fputs("\n", stdout);
			}
		}
		
		repo.finalizePack(snapshot, packState);
	}
	
	repo.commitSnapshot(snapshot, snapshotName);
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
	
	std::shared_ptr<Snapshot> snapshot(repo.loadSnapshot(snapshotName));
	snapshot->forEachFileEntry([&snapshot] (const Snapshot::FileEntry &fe) {
		const char *user = snapshot->indexToString(fe.userIndex);
		const char *group = snapshot->indexToString(fe.groupIndex);
		filesystem::path name = snapshot->indexToString(fe.nameIndex);
		filesystem::path path = snapshot->indexToString(fe.pathIndex);

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
			   (path / name).c_str());
	});
}

static void downloadFiles(const char *repository, const char *snapshotName, int argc, const char * const *argv)
{
	using namespace Nebula;
	using namespace boost;
	
	if(!filesystem::is_directory(argv[argc - 1])) {
		throw InvalidArgumentException("Last argument should specify a valid directory.");
	}
	filesystem::path destDir = filesystem::path(argv[argc - 1]);

	const char *argv2[] = { "", destDir.c_str() };
	if(argc == 1) {
		argv = argv2;
		argc++;
	}

	auto dataStore = createDataStoreFromRepository(repository);
	Repository repo(dataStore.get());
	
	ZeroedString password = promptReadPassword(false);
	if(!repo.unlockRepository(password.c_str())) {
		throw RepositoryException("Invalid password.");
	}
	
	std::shared_ptr<Snapshot> snapshot(repo.loadSnapshot(snapshotName));
	for(int i = 0; i < argc - 1; ++i) {
		const char * srcFile = argv[i];

		snapshot->forEachFileEntry(srcFile,
			[&repo, srcFile, &snapshot, &destDir](const Snapshot::FileEntry& fe) {
			std::string name = snapshot->indexToString(fe.nameIndex);
			std::string path = snapshot->indexToString(fe.pathIndex);

			filesystem::path inputParentPath = filesystem::path(srcFile).parent_path();
			filesystem::path destFilePath = filesystem::relative(path + "/" + name, inputParentPath);

			filesystem::path filePath = destDir / filesystem::path(destFilePath);

			if(!options.force && !options.dryRun && filesystem::exists(filePath)) {
				printf("%s: File exists. Overwrite (y/n)? ", filePath.c_str());
				fflush(stdout);
				if(fgetc(stdin) != 'y') {
					return;
				}
				if(!options.quiet) printf("\n");
			}
			
			if(!options.quiet) {
				printf("%s\n", filePath.c_str());
			}

			if(!options.dryRun) {
				if(!filesystem::is_directory(filePath.parent_path())) {
					filesystem::create_directories(filePath.parent_path());
				}

				{
					time_t startTime = time(nullptr);
					FileStream outStream(filePath.c_str(), FileMode::Write);
					int lastBlockNo = 0;
					repo.downloadFile(snapshot, (path + "/" + name).c_str(), outStream,
						[startTime, &lastBlockNo](int blockNo, int blockCount, long bytesDownloaded, long bytesTotal) -> bool {
							if(lastBlockNo != blockNo) {
								lastBlockNo = blockNo;
								fputs("\n", stdout);
							}
							printProgress(blockNo, blockCount, bytesDownloaded, bytesTotal, startTime);
							return !sUserCancelled;
						});
					if(!options.quiet) printf("\n");
				}
				
				if(options.verify) {
					uint8_t buffer[8192];
					size_t n;

					FileStream inStream(filePath.c_str(), FileMode::Read);
					
					EVP_MD_CTX ctx;
					EVP_MD_CTX_init(&ctx);
					std::unique_ptr<EVP_MD_CTX, decltype(EVP_MD_CTX_cleanup) *>
						cleanup(&ctx, EVP_MD_CTX_cleanup);

					if(!EVP_DigestInit(&ctx, EVP_md5())) {
						throw VerificationFailedException("EVP_DigestInit() failed.");
					}

					while((n = inStream.read(buffer, sizeof(buffer))) > 0) {
						if(!EVP_DigestUpdate(&ctx, buffer, n)) {
							throw VerificationFailedException("EVP_DigestUpdate() failed.");
						}
					}

					uint8_t md[MD5_DIGEST_LENGTH];
					if(!EVP_DigestFinal(&ctx, md, nullptr)) {
						throw VerificationFailedException("EVP_DigestFinal() failed.");
					}

					if(memcmp(md, fe.md5, MD5_DIGEST_LENGTH) != 0) {
						std::strstream str;
						str << filePath.string() << ": File verification failed. ";
						throw VerificationFailedException(str.str());
					}
				}
			}
		});
		
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
		{ "dry-run", no_argument, 0, 'n' },
		{ "no-verify", no_argument, 0, 0 },
		{ "backend", required_argument, 0, 'b' },
		{ 0, 0, 0, 0 }
	};
	
	int c;
	int optIndex;
	while((c = getopt_long(argc, argv, "qnfb:", longOptions, &optIndex)) >= 0) {
		switch (c) {
			case 0:
				if(strcmp(longOptions[optIndex].name, "no-verify") == 0) {
					options.verify = false;
				}
				break;
			case 'q':
				options.quiet = true;
				break;
			case 'n':
				options.dryRun = true;
				break;
			case 'f':
				options.force = true;
				break;
			case 'b':
				options.backend = optarg;
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

	signal(SIGINT, sigHandler);
	signal(SIGQUIT, sigHandler);
	signal(SIGTERM, sigHandler);
	
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
			case 'r':
				if(strcmp(action, "restore") == 0) {
					if(optind + 3 > argc) {
						throw InvalidArgumentException("A snapshot name must be specified.");
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
		fprintf(stderr, "\nerror: %s\n", e.what());
	}

	return 0;
}
