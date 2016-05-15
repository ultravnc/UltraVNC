/* wsfdata.h
 * 
 * This file was created by an automated utility. 
 * It is not intended for manual editing
 */

extern em_file efslist[38];

extern  unsigned char index_html1[1315];
extern  unsigned char passwd_html2[1383];
extern  unsigned char settings_html3[3876];
extern  unsigned char ok_html4[513];
extern  unsigned char nok_html5[528];
extern  unsigned char log_html6[867];
extern  unsigned char comment_html7[1923];
extern  unsigned char images2_jpg8[1702];
extern  unsigned char connections_txt9[39];
extern  unsigned char viewer_access_txt10[41];
extern  unsigned char server_access_txt11[41];

int     memory_ssi(wi_sess * sess, EOFILE * eofile);

int     mode1_ssi(wi_sess * sess, EOFILE * eofile);

int     mode2_ssi(wi_sess * sess, EOFILE * eofile);

int     sport_ssi(wi_sess * sess, EOFILE * eofile);

int     vport_ssi(wi_sess * sess, EOFILE * eofile);

int     acon_ssi(wi_sess * sess, EOFILE * eofile);

int     rcon_ssi(wi_sess * sess, EOFILE * eofile);

int     aid_ssi(wi_sess * sess, EOFILE * eofile);

int     acons_ssi(wi_sess * sess, EOFILE * eofile);

int     rcons_ssi(wi_sess * sess, EOFILE * eofile);

int     aids_ssi(wi_sess * sess, EOFILE * eofile);

int     webport_ssi(wi_sess * sess, EOFILE * eofile);

int     log_ssi(wi_sess * sess, EOFILE * eofile);

int     ucom_ssi(wi_sess * sess, EOFILE * eofile);

int     listcomment_ssi(wi_sess * sess, EOFILE * eofile);

int     connections_ssi(wi_sess * sess, EOFILE * eofile);

int     server_access_ssi(wi_sess * sess, EOFILE * eofile);

int     viewer_access_ssi(wi_sess * sess, EOFILE * eofile);

int     keepalive_ssi(wi_sess * sess, EOFILE * eofile);

int     pushtest_func(wi_sess * sess, EOFILE * eofile);

char *  testaction_cgi(wi_sess * sess, EOFILE * eofile);

char *  testaction2_cgi(wi_sess * sess, EOFILE * eofile);

char *  testaction3_cgi(wi_sess * sess, EOFILE * eofile);

char *  testaction4_cgi(wi_sess * sess, EOFILE * eofile);

char *  testaction5_cgi(wi_sess * sess, EOFILE * eofile);

char *  passwd_cgi(wi_sess * sess, EOFILE * eofile);


#define  MEMHITS_VAR31                    31


