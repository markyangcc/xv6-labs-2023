#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *fmtname(char *path) //格式化名字，把名字变成前面没有左斜杠/，仅仅保存文件名
{
	static char buf[DIRSIZ + 1];
	char *p;

	// Find first character after last slash.
	for (p = path + strlen(path); p >= path && *p != '/'; p--)
		;
	p++;

	// Return blank-padded name.
	memmove(buf, p, strlen(p) + 1);
	return buf;
}

void find(char *dir, char *name) {
	char path[512];
	char *p;
	int fd;
	struct dirent de;
	struct stat st;

	if ((fd = open(dir, 0)) < 0) {
		fprintf(2, "find: cannot open %s\n", dir);
		return;
	}

	if (fstat(fd, &st) < 0) {
		fprintf(2, "find: cannot stat %s\n", dir);
		close(fd);
		return;
	}

	switch (st.type) {
	case T_FILE:
		if (strcmp(fmtname(dir), name) == 0)
			printf("%s\n", dir);
		break;
	case T_DIR:
		if (strlen(dir) + 1 + DIRSIZ + 1 > sizeof path) {
			printf("find: path too long\n");
			break;
		}

		strcpy(path, dir);
		p = path + strlen(path);
		*p++ = '/'; // path 追加 '/'

		// Hint: 目录也是一个 file，循环读取一个 struct dirent 大小，就可以遍历目录下所有文件
		while (read(fd, &de, sizeof(de)) == sizeof(de)) {
			if (de.inum == 0) {
				continue;
			}

			memmove(p, de.name, DIRSIZ); // memmove把 de.name 信息追加到 path
			p[strlen(de.name)] = 0;		 // EOF

			// ignore '.' and '..'
			if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
				continue;

			find(path, name);
		}
		break;
	}
	close(fd);
}

int main(int argc, char *argv[]) {

	if (argc != 3) {
		printf("Usage: find [path] [name]");
		exit(0);
	}

	find(argv[1], argv[2]);
	exit(0);
}
