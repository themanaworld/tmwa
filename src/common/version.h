/// Some constants to identify the version of (e)Athena
/// The values are different if the client connects (-1,'T','M','W',flags32)
// These numbers have never been changed while TMW
#ifndef VERSION_H
#define VERSION_H
//When a server receives a 0x7530 packet from an admin connection,
//it sends an 0x7531 packet with the following bytes
# define ATHENA_MAJOR_VERSION    1   // Major Version
# define ATHENA_MINOR_VERSION    0   // Minor Version
# define ATHENA_REVISION         0   // Revision

# define ATHENA_RELEASE_FLAG     1   // 1=Develop,0=Stable
# define ATHENA_OFFICIAL_FLAG    1   // 1=Mod,0=Official

// and a bitmask of these (the char server sends char and inter)
# define ATHENA_SERVER_LOGIN     1   // login server
# define ATHENA_SERVER_CHAR      2   // char server
# define ATHENA_SERVER_INTER     4   // inter server
# define ATHENA_SERVER_MAP       8   // map server

// and this as two bytes
# define ATHENA_MOD_VERSION   1052   // mod version (patch No.)

#endif // VERSION_H
