/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sqliterk_os.h"
#include "sqliterk_util.h"
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

//数据库文件结构体
struct sqliterk_file {
    char *path;       //文件路径
    int fd;           //文件句柄
    int error; // errno will be set when system error occur
};

//使用只读方式打开数据库文件
int sqliterkOSReadOnlyOpen(const char *path, sqliterk_file **file)
{
    if (!path || !path[0] || !file) {
        return SQLITERK_MISUSE;
    }
    int rc = SQLITERK_OK;
    //申请内存，创建数据库文件结构体
    sqliterk_file *theFile = sqliterkOSMalloc(sizeof(sqliterk_file));
    if (!theFile) {
        rc = SQLITERK_NOMEM;
        sqliterkOSError(rc, "Not enough memory, required %u bytes.",
                        sizeof(sqliterk_file));
        goto sqliterkOSReadOnlyOpen_Failed;
    }

    size_t len = sizeof(char) * (strlen(path) + 1);
    theFile->path = sqliterkOSMalloc(len);
    if (!theFile->path) {
        rc = SQLITERK_NOMEM;
        sqliterkOSError(rc, "Not enough memory, required %u bytes.", len);
        goto sqliterkOSReadOnlyOpen_Failed;
    }
    //设置theFile的文件路径
    strncpy(theFile->path, path, len);

    // Open the file in read-only mode, since we do not intend to modify it
    //使用只读方式打开，获取文件句柄
    theFile->fd = open(theFile->path, O_RDONLY);
    if (theFile->fd < 0) {
        rc = SQLITERK_CANTOPEN;
        sqliterkOSError(rc, "Cannot open '%s' for reading: %s", theFile->path,
                        strerror(errno));
        goto sqliterkOSReadOnlyOpen_Failed;
    }
    *file = theFile;
    return SQLITERK_OK;

sqliterkOSReadOnlyOpen_Failed:
    if (theFile) {
        sqliterkOSClose(theFile);
    }
    *file = NULL;
    return rc;
}

//关闭指定文件
int sqliterkOSClose(sqliterk_file *file)
{
    if (!file) {
        return SQLITERK_MISUSE;
    }
    //释放路径字符串
    if (file->path) {
        sqliterkOSFree((char *) file->path);
        file->path = NULL;
    }
    //关闭文件
    if (file->fd >= 0) {
        close(file->fd);
        file->fd = -1;
    }
    file->error = 0;
    //释放文件结构体
    sqliterkOSFree(file);
    return SQLITERK_OK;
}

//读取指定文件数据
int sqliterkOSRead(sqliterk_file *file,
                   off_t offset,
                   unsigned char *data,
                   size_t *size)
{
    if (!file || file->fd < 0) {
        return SQLITERK_MISUSE;
    }
    /*
     1) 欲将读写位置移到文件开头时:
     lseek（int fildes,0,SEEK_SET）；
     2) 欲将读写位置移到文件尾时:
     lseek（int fildes，0,SEEK_END）；
     3) 想要取得目前文件位置时:
     lseek（int fildes，0,SEEK_CUR）；
     //http://baike.baidu.com/link?url=0tb5uqdjRVm3EYnrZIUg44Ds8Kp6_eOfwKmn5zrRGXX2jpdFwnSa7wgBZEkBQ0gYn12tkW81q-BxB2XS4L5aHK
     */
    //移动文件的偏移量，方便下次读取
    off_t newOffset = lseek(file->fd, offset, SEEK_SET);
    if (newOffset == -1) {
        file->error = errno;
        return SQLITERK_IOERR;
    }
    //获取size的值
    size_t left = *size;
    size_t cnt = 0;
    ssize_t got = 0;
    do {
        //读取文件，返回读取大小
        got = read(file->fd, data, left);
        if (got < 0) {
            //如果错误代码等于输入，则退出循环
            if (errno == EINTR) {
                got = 1;
                continue;
            }
            //其他错误，则退出函数
            file->error = errno;
            return SQLITERK_IOERR;
        } else if (got > 0) {
            //读取成功，需要读取的size减去已经读的size.读取大小加上已经读的size.data便宜到后续需要写入位置
            left -= got;
            cnt += got;
            data = data + got;
        }
    } while (got > 0 && left > 0);
    *size = cnt;
    if (left > 0) {
        return SQLITERK_SHORT_READ;
    }
    return SQLITERK_OK;
}

//获取文件大小
int sqliterkOSFileSize(sqliterk_file *file, size_t *filesize)
{
    if (!file || file->fd < 0) {
        return SQLITERK_MISUSE;
    }
    //保存文件状态的结构体
    struct stat statbuf;
    //stat和lstat的区别：当文件是一个符号链接时，lstat返回的是该符号链接本身的信息；而stat返回的是该链接指向的文件的
    //fstat区别于另外两个系统调用的地方在于，fstat系统调用接受的是 一个“文件描述符”，而另外两个则直接接受“文件全路径”。文件描述符是需要我们用open系统调用后才能得到的，而文件全路经直接写就可以了。
    //读取文件状态
    if (fstat(file->fd, &statbuf) != 0) {
        file->error = errno;
        return SQLITERK_IOERR;
    }
    //返回文件大小
    *filesize = (size_t) statbuf.st_size;
    return SQLITERK_OK;
}

//获取文件路径
const char *sqliterkOSGetFilePath(sqliterk_file *file)
{
    return file->path;
}

//申请内存
void *sqliterkOSMalloc(size_t size)
{
    return calloc(size, sizeof(char));
}

//释放内存
void sqliterkOSFree(void *p)
{
    free(p);
}

//默认的log函数
static void
sqliterkDefaultLog(sqliterk_loglevel level, int result, const char *msg)
{
    fprintf(stderr, "[%s] %s\n", sqliterkGetResultCodeDescription(result), msg);
}

//设置最大log数，设置默认os的Log函数
#define SQLITRK_CONFIG_MAXLOG 4096
static sqliterk_os s_os = {sqliterkDefaultLog};

//使用可变参函数，实现log函数
int sqliterkOSLog(sqliterk_loglevel loglevel,
                  int result,
                  const char *format,
                  ...)
{
    char buf[SQLITRK_CONFIG_MAXLOG];

    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    if (s_os.xLog) {
        s_os.xLog(loglevel, result, buf);
    }
    return result;
}

//注册os
int sqliterkOSRegister(sqliterk_os os)
{
    s_os = os;
    return SQLITERK_OK;
}
