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
#include "libnebula/Exception.h"
#include "libnebula/Repository.h"

static void printHelp()
{
	printf("usage: NebulaBackup [options] init <repo>\n");
	printf("       NebulaBackup [options] create <repo> <snapshot> <file> [<file>...]\n");
	printf("       NebulaBackup [options] download <repo> <snapshot> <file> [<file>...]\n");
	printf("       NebulaBackup [options] list <repo>\n");
	printf("       NebulaBackup [options] list <repo> <snapshot>\n");
	printf("       NebulaBackup [options] delete <repo> <snapshot>\n");
	printf("       NebulaBackup [options] compact <repo>\n");
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

std::shared_ptr<Nebula::DataStore> createDataStoreFromRepository(const char *repo)
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

Nebula::ZeroedString promptReadPassword()
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
	fgets(passwd1, sizeof(passwd1), stdin);
	printf("\n");
	printf("Confirm password: ");
	fgets(passwd2, sizeof(passwd2), stdin);
	printf("\n");

	size_t n = strnlen(passwd1, sizeof(passwd1));
	while (passwd1[--n] == '\n') {
		passwd1[n] = 0;
	}

	outPassword = passwd1;

	return outPassword;
}

int main(int argc, char *argv[])
{
	using namespace Nebula;

	static struct option longOptions[] =
	{
		{ "help", no_argument, 0, 0 },
		{ "quiet", no_argument, 0, 'q' },
		{ 0, 0, 0, 0 }
	};promptReadPassword();
	
	int c;
	int optIndex;
	while((c = getopt_long(argc, argv, "q", longOptions, &optIndex)) >= 0) {
		switch (c) {
			case 0:
				
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
					fprintf(stderr, "error: Invalid action '%s'\n", action);
					printHelp();
					exit(-1);
				}
				
				auto dataStore = createDataStoreFromRepository(repo);
				Repository repo(dataStore.get());
				
				
				//repo.initializeRepository(<#const char *password#>);
				break;
			}
		}
	} catch(const std::exception& e) {
		fprintf(stderr, "error: %s\n", e.what());
	}

	return 0;
}
