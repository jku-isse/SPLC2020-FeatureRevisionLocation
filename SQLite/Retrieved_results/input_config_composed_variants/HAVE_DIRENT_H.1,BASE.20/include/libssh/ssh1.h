
#define __SSH1_H 

#define SSH_MSG_NONE 0
#define SSH_MSG_DISCONNECT 1
#define SSH_SMSG_PUBLIC_KEY 2
#define SSH_CMSG_SESSION_KEY 3
#define SSH_CMSG_USER 4
#define SSH_CMSG_AUTH_RHOSTS 5
#define SSH_CMSG_AUTH_RSA 6
#define SSH_SMSG_AUTH_RSA_CHALLENGE 7
#define SSH_CMSG_AUTH_RSA_RESPONSE 8
#define SSH_CMSG_AUTH_PASSWORD 9
#define SSH_CMSG_REQUEST_PTY 10
#define SSH_CMSG_WINDOW_SIZE 11
#define SSH_CMSG_EXEC_SHELL 12
#define SSH_CMSG_EXEC_CMD 13
#define SSH_SMSG_SUCCESS 14
#define SSH_SMSG_FAILURE 15
#define SSH_CMSG_STDIN_DATA 16
#define SSH_SMSG_STDOUT_DATA 17
#define SSH_SMSG_STDERR_DATA 18
#define SSH_CMSG_EOF 19
#define SSH_SMSG_EXITSTATUS 20
#define SSH_MSG_CHANNEL_OPEN_CONFIRMATION 21
#define SSH_MSG_CHANNEL_OPEN_FAILURE 22
#define SSH_MSG_CHANNEL_DATA 23
#define SSH_MSG_CHANNEL_CLOSE 24
#define SSH_MSG_CHANNEL_CLOSE_CONFIRMATION 25
/*      SSH_CMSG_X11_REQUEST_FORWARDING		26	   OBSOLETE */
#define SSH_SMSG_X11_OPEN 27
#define SSH_CMSG_PORT_FORWARD_REQUEST 28
#define SSH_MSG_PORT_OPEN 29
#define SSH_CMSG_AGENT_REQUEST_FORWARDING 30
#define SSH_SMSG_AGENT_OPEN 31
#define SSH_MSG_IGNORE 32
#define SSH_CMSG_EXIT_CONFIRMATION 33
#define SSH_CMSG_X11_REQUEST_FORWARDING 34
#define SSH_CMSG_AUTH_RHOSTS_RSA 35
#define SSH_MSG_DEBUG 36
#define SSH_CMSG_REQUEST_COMPRESSION 37
#define SSH_CMSG_MAX_PACKET_SIZE 38
#define SSH_CMSG_AUTH_TIS 39
#define SSH_SMSG_AUTH_TIS_CHALLENGE 40
#define SSH_CMSG_AUTH_TIS_RESPONSE 41
#define SSH_CMSG_AUTH_KERBEROS 42
#define SSH_SMSG_AUTH_KERBEROS_RESPONSE 43
#define SSH_CMSG_HAVE_KERBEROS_TGT 44
#define SSH_CMSG_HAVE_AFS_TOKEN 65

/* protocol version 1.5 overloads some version 1.3 message types */
#define SSH_MSG_CHANNEL_INPUT_EOF SSH_MSG_CHANNEL_CLOSE
#define SSH_MSG_CHANNEL_OUTPUT_CLOSE SSH_MSG_CHANNEL_CLOSE_CONFIRMATION

/*
 * Authentication methods.  New types can be added, but old types should not
 * be removed for compatibility.  The maximum allowed value is 31.
 */
#define SSH_AUTH_RHOSTS 1
#define SSH_AUTH_RSA 2
#define SSH_AUTH_PASSWORD 3
#define SSH_AUTH_RHOSTS_RSA 4
#define SSH_AUTH_TIS 5
#define SSH_AUTH_KERBEROS 6
#define SSH_PASS_KERBEROS_TGT 7
				/* 8 to 15 are reserved */
#define SSH_PASS_AFS_TOKEN 21

/* Protocol flags.  These are bit masks. */
#define SSH_PROTOFLAG_SCREEN_NUMBER 1
#define SSH_PROTOFLAG_HOST_IN_FWD_OPEN 2

/* cipher flags. they are bit numbers */
#define SSH_CIPHER_NONE 0
#define SSH_CIPHER_IDEA 1
#define SSH_CIPHER_DES 2
#define SSH_CIPHER_3DES 3
#define SSH_CIPHER_RC4 5
#define SSH_CIPHER_BLOWFISH 6



