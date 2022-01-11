bool
lws_read_float(gfloat* f);
bool
lws_read_int(int32* i);
bool
lws_read_uint(uint32* i);
char* lws_read_name(char* s);
char*
lws_read_string();
int32
lws_get_token(void);
void
lws_bad_token(int32 t);
void
lws_unget_token(void);
bool
lws_check_token(int32 t);
void BadToken(char *string, char *token);
