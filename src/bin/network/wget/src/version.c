/* version.c */
/* Autogenerated by Makefile - DO NOT EDIT */

const char *version_string = "1.12"
;
const char *compilation_string = "i586-pc-haiku-gcc -DHAVE_CONFIG_H -DSYSTEM_WGETRC=\"/boot/common/etc/wgetrc\" -DLOCALEDIR=\"/boot/common/share/locale\" -I. -I../lib -I ../md5 -I/boot/common/include -O2 -Wall";
const char *link_string = "i586-pc-haiku-gcc -O2 -Wall /boot/common/lib/libiconv.so -Wl,-rpath -Wl,/boot/common/lib ftp-opie.o gen-md5.o ../lib/libgnu.a ../md5/libmd5.a";
