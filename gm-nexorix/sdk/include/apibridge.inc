#if defined _apibridge_included
  #endinput
#endif
#define _apibridge_included

#define API_METHOD_GET "GET"
#define API_METHOD_POST "POST"
#define API_METHOD_PUT "PUT"
#define API_METHOD_DELETE "DELETE"

native Api_Init();
native Api_Request(const method[], const url[], const body[], const callback[], const extraData = 0);
native Api_SetHeader(const key[], const value[]);
native Api_ClearHeaders();
native Api_EncryptAES(const input[], const key[], output[], outputLen);
native Api_DecryptAES(const input[], const key[], output[], outputLen);
native Api_GenerateHMAC(const data[], const secret[], output[], outputLen);
native Api_CreateJWT(const payloadJson[], const secret[], output[], outputLen);
native Api_VerifyJWT(const token[], const secret[]);
native Api_Shutdown();

forward OnApiResponse(responseCode, const responseBody[], extraData);
