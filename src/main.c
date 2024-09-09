//usr/bin/env tcc -DPROC_COUNT=4 $(ls *.c | grep -v main.c) -run "$0" "$@" ; exit $?

#include <stdio.h>
#include <errno.h>

#include "file.h"
#include "platform.h"
#include "err.h"
#include "arg.h"
#include "str.h"
#include "info.h"


#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <utime.h>

typedef struct FileDate {
    Str filename;
    time_t date;
} FileDate;

void filedate_free(FileDate *fd)
{
    str_free(&fd->filename);
    memset(fd, 0, sizeof(*fd));
}

VEC_INCLUDE(VFileDate, vfiledate, FileDate, BY_REF, BASE);
VEC_INCLUDE(VFileDate, vfiledate, FileDate, BY_REF, SORT);

int filedate_cmp(FileDate *a, FileDate *b)
{
    return a->date - b->date;
}

VEC_IMPLEMENT(VFileDate, vfiledate, FileDate, BY_REF, BASE, filedate_free);
VEC_IMPLEMENT(VFileDate, vfiledate, FileDate, BY_REF, SORT, filedate_cmp);

//#include <ctype.h>

int check_file(Str *file, void *data)
{
    size_t iE = str_rch(file, '/', 0);
    if(iE >= str_length(file)) THROW(ERR_UNREACHABLE);
    Str base = STR_I0(*file, iE + 1);
    if(str_length(&base) != 16) return 0;
    const Str match0 = STR("mpv-shot");
    const Str matchE = STR(".jpg");
    if(str_cmp(&STR_IE(base, str_length(&match0)), &match0)) return 0;
    if(str_cmp(&STR_I0(base, str_length(&match0)+4), &matchE)) return 0;
    //printf("%.*s\n", STR_F(&base));
    FileDate fd = {0};
    TRYC(str_copy(&fd.filename, file));
    VFileDate *arr = data;

    struct stat s;
    int r = lstat(fd.filename.s, &s);
    if(r) return -1;
    fd.date = s.st_mtime;

    TRY(vfiledate_push_back(arr, &fd), ERR_VEC_PUSH_BACK);
    return 0;
error:
    return -1;
}

int file_move(Str *src, Str *dst)
{
    struct stat file_stat;
    
    // Step 1: Get the original file's timestamps
    if (stat(src->s, &file_stat) != 0) {
        info_check(INFO_rename, false);
        perror(">>> Stat Failed");
        return -1;
    }

    // Step 2: Move (rename) the file
    if (rename(src->s, dst->s) != 0) {
        info_check(INFO_rename, false);
        perror(">>> Rename Failed");
        return -1;
    }
    info_check(INFO_rename, true);

    // Step 3: Set the original file's timestamps on the new file
    struct utimbuf new_times;
    new_times.actime = file_stat.st_atime;  // Access time
    new_times.modtime = file_stat.st_mtime; // Modification time

    if (utime(dst->s, &new_times) != 0) {
        perror("utime failed");
        return -1;
    }

    return 0; // Success
}

int main(int argc, const char **argv)
{
    int err = 0;

    info_disable_all(INFO_LEVEL_ALL);
    info_enable(INFO_rename, INFO_LEVEL_TEXT);

    Str source = {0};
    Str destination = {0};
    Str outfolder = {0};
    Str tmpfolder = {0};
    VFileDate origins = {0};
    VStr subdirs = {0};

    Arg arg = {0};
    TRYC(arg_parse(&arg, argc, argv));
    if(arg.exit_early) goto clean;

    str_trim(&arg.parsed.input);
    str_trim(&arg.parsed.output);
    if(!str_length(&arg.parsed.input)) {
        THROW("expecting input");
    }
    if(!str_length(&arg.parsed.output)) {
        THROW("expecting output");
    }


    TRYC(file_exec(&arg.parsed.input, &subdirs, true, check_file, &origins));
    while(vstr_length(&subdirs)) {
        Str pop = {0};
        vstr_pop_back(&subdirs, &pop);
        memset(subdirs.items[vstr_length(&subdirs)], 0, sizeof(Str)); // TODO: this should probably happen in my vector!
        TRYC(file_exec(&pop, &subdirs, true, check_file, &origins));
        str_free(&pop);
    }

    vfiledate_sort(&origins);

    TRYC(str_copy(&outfolder, &arg.parsed.output));
    if(str_get_back(&outfolder) != '/') {
        TRYC(str_push_back(&outfolder, '/'));
    }


    char const *folder = getenv("TMPDIR");
    if (folder == 0) {
        folder = "/tmp";
    }
    TRYC(str_fmt(&tmpfolder, "%s/mpv-shotXXXX/", folder));

    struct stat st = {0};
    if (stat(tmpfolder.s, &st) == -1) {
        mkdir(tmpfolder.s, 0700);
    }
    
    if(stat(outfolder.s, &st) == -1) {
        THROW("output folder '%.*s' doesn't exist", STR_F(&outfolder));
    }


    size_t n = 1;
    for(size_t i = 0; i < vfiledate_length(&origins); ++i, ++n) {
        str_clear(&destination);
        TRYC(str_fmt(&destination, "%.*smpv-shot%04zu.jpg", STR_F(&tmpfolder), n));
        FileDate *origin = vfiledate_get_at(&origins, i);
        info(INFO_rename, "'%.*s' <- '%.*s' (%zu)", STR_F(&destination), STR_F(&origin->filename), origin->date);
        int r = file_move(&origin->filename, &destination);
        if(r) {
            if(errno == EISDIR) {
                //printff("'%.*s' is a directory", STR_F(&destination));
                fprintf(stderr, ">>> Retrying on Next ...\n");
                --i;
            } else {
                THROW("unhandled errno: %u", errno);
            }
        } else {
        }
    }

    //if(stat(outfolder.s, &st) == -1) {
    //    mkdir(outfolder.s, 0700);
    //}

    n = 1;
    for(size_t i = 0; i < vfiledate_length(&origins); ++i, ++n) {
        str_clear(&source);
        str_clear(&destination);
        TRYC(str_fmt(&source, "%.*smpv-shot%04zu.jpg", STR_F(&tmpfolder), i + 1));
        TRYC(str_fmt(&destination, "%.*smpv-shot%04zu.jpg", STR_F(&outfolder), n));
        FileDate *origin = vfiledate_get_at(&origins, i);
        if(0) continue;
        info(INFO_rename, "'%.*s' <- '%.*s' (%zu)", STR_F(&destination), STR_F(&source), origin->date);
        int r = file_move(&source, &destination);
        if(r) {
            if(errno == EISDIR) {
                //printff("'%.*s' is a directory", STR_F(&destination));
                --i;
                fprintf(stderr, ">>> Retrying on Next ...\n");
            } else {
                THROW("unhandled errno: %u", errno);
            }
        } else {
        }
    }

clean:
    str_free(&tmpfolder);
    str_free(&source);
    str_free(&destination);
    str_free(&outfolder);
    vfiledate_free(&origins);
    vstr_free(&subdirs);
    arg_free(&arg);
    info_handle_abort();
    return err;
error:
    ERR_CLEAN;
}

