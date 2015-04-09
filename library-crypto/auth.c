/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * (c)Copyright 2006 Hewlett-Packard Development Company, LP.
 *
 *  Extracted from sipp project : http://sipp.sourceforge.net
 *
 */

#ifdef __FreeBSD__
#include <sys/types.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <openssl/md5.h>
#include "milenage.h"

#include "Utils.hpp"

#define MAX_HEADER_LEN  2049
#define MD5_HASH_SIZE 16
#define HASH_HEX_SIZE 2*MD5_HASH_SIZE


/* AKA */

#define KLEN 16
typedef u_char K[KLEN];
#define RANDLEN 16
typedef u_char RAND[RANDLEN];
#define AUTNLEN 16
typedef u_char AUTN[AUTNLEN];

#define AKLEN 6
typedef u_char AK[AKLEN];
#define AMFLEN 2
typedef u_char AMF[AMFLEN];
#define MACLEN 8
typedef u_char MAC[MACLEN];
#define CKLEN 16
typedef u_char CK[CKLEN];
#define IKLEN 16
typedef u_char IK[IKLEN];
#define SQNLEN 6
typedef u_char SQN[SQNLEN];
#define AUTSLEN 14
typedef char AUTS[AUTSLEN];
#define AUTS64LEN 29
typedef char AUTS64[AUTS64LEN];
#define RESLEN 8
typedef unsigned char RES[RESLEN+1];
#define RESHEXLEN 17
typedef char RESHEX[RESHEXLEN];
#define OPLEN 16
typedef u_char OP[OPLEN];

AMF amfstar= { '\0' , '\0'} ;
SQN sqn_he={0x00,0x00,0x00,0x00,0x00,0x00};

/* end AKA */


int createAuthHeaderMD5(char * user, char * password, char * method,
                     char * uri, char * msgbody, char * auth, 
                     char * algo, char * result);
int createAuthHeaderAKAv1MD5(char * user, char * OP,
                             char * AMF,
                             char * K,
                             char * method,
                             char * uri, char * msgbody, char * auth, char *algo,
                             char * result);


/* This function is from RFC 2617 Section 5 */

void hashToHex (unsigned char *_b, unsigned char *_h)
{
  unsigned short i;
  unsigned char j;
  
  for (i = 0; i < MD5_HASH_SIZE; i++) {
    j = (_b[i] >> 4) & 0xf;
    if (j <= 9) {
      _h[i * 2] = (j + '0');
    } else {
      _h[i * 2] = (j + 'a' - 10);
    }
    j = _b[i] & 0xf;
    if (j <= 9) {
      _h[i * 2 + 1] = (j + '0');
    } else {
      _h[i * 2 + 1] = (j + 'a' - 10);
    }
  };
  _h[HASH_HEX_SIZE] = '\0';
}

char *stristr (const char *s1, const char *s2) {
  char *cp = (char*) s1;
  char *p1, *p2, *endp;
  char l, r;
  
  endp = (char*)s1 + (strlen(s1) - strlen(s2)) ;
  while (*cp && (cp <= endp)) {
    p1 = cp;
    p2 = (char*)s2;
    while (*p1 && *p2) {
      l = toupper(*p1);
      r = toupper(*p2); 
      if (l != r) {
        break;
      }
      p1++;
      p2++;
    }
    if (*p2 == 0) {
      return cp;
    }
    cp++;
  }
  return 0;
}

int createAuthHeader(char * user, char * password, char * method,
                     char * uri, char * msgbody, char * auth, 
                     char * aka_OP,
                     char * aka_AMF,
                     char * aka_K,
                     char * result) {
  
  char algo[32]="MD5";
  char *start, *end;
  
  if ((start = stristr(auth, "Digest")) == NULL) {
    sprintf(result, "createAuthHeader: authentication must be digest");
    return 0;
  }
  
  if ((start = stristr(auth, "algorithm=")) != NULL) {
    start = start + strlen("algorithm=");
    if (*start == '"') { start++; }
    end = start + strcspn(start, " ,\"\r\n");
    strncpy(algo, start, end - start);
    algo[end - start] ='\0';
    
  }
  
  if (strncasecmp(algo, "MD5", 3)==0) {
    return createAuthHeaderMD5(user,password,method,uri,msgbody,auth,algo,result);
  } else if (strncasecmp(algo, "AKAv1-MD5", 9)==0) {
    return createAuthHeaderAKAv1MD5(user, aka_OP,
                                    aka_AMF,
                                    aka_K,
                                    method,uri,msgbody,auth,algo,result);        
  }else{
    sprintf(result, "createAuthHeader: authentication must use MD5 or AKAv1-MD5");
    return 0;
  }
}


int createAuthHeaderMD5(char * user, char * password, char * method,
                        char * uri, char * msgbody, char * auth, 
                        char * algo, char * result) {
  
  unsigned char ha1[MD5_HASH_SIZE], ha2[MD5_HASH_SIZE];
  unsigned char resp[MD5_HASH_SIZE], body[MD5_HASH_SIZE]; 
  unsigned char ha1_hex[HASH_HEX_SIZE+1], ha2_hex[HASH_HEX_SIZE+1];
  unsigned char resp_hex[HASH_HEX_SIZE+1], body_hex[HASH_HEX_SIZE+1];
  char tmp[MAX_HEADER_LEN], authtype[16], cnonce[32], nc[32], opaque[64];
  char *start, *end;
  static unsigned int mync = 1;
  int has_opaque = 0;
  MD5_CTX Md5Ctx;
  
  // Extract the Auth Type - If not present, using 'none' 
  cnonce[0] = '\0';
  authtype[0] = '\0';
  if ((start = stristr(auth, "qop=")) != NULL) {
    start = start + strlen("qop=");
    if (*start == '"') { start++; }
    end = start + strcspn(start, " ,\"\r\n");
    strncpy(authtype, start, end - start);
    authtype[end - start] ='\0';
    sprintf(cnonce, "%x", rand());
    sprintf(nc, "%08x", mync);
  }
  
  // Extract the Opaque value - if present
  opaque[0] = '\0';
  if ((start = stristr(auth, "opaque=")) != NULL) {
    start = start + strlen("opaque=");
    if (*start == '"') { start++; }
    end = start + strcspn(start, " ,\"\r\n");
    strncpy(opaque, start, end - start);
    opaque[end - start] ='\0';
    has_opaque = 1;
  }
  
  // Extract the Realm 
  if ((start = stristr(auth, "realm=")) == NULL) {
    sprintf(result, "createAuthHeaderMD5: couldn't parse realm in '%s'", auth);
    return 0;
  }
  start = start + strlen("realm=");
  if (*start == '"') { start++; }       
  end = start + strcspn(start, ",\"\r\n");
  strncpy(tmp, start, end - start);
  tmp[end - start] ='\0';
  
  // Load in A1 
  MD5_Init(&Md5Ctx);
  MD5_Update(&Md5Ctx, user, strlen(user));
  MD5_Update(&Md5Ctx, ":", 1);
  MD5_Update(&Md5Ctx, tmp, strlen(tmp));
  MD5_Update(&Md5Ctx, ":", 1);
  MD5_Update(&Md5Ctx, password, strlen(password));
  MD5_Final(ha1, &Md5Ctx);
  hashToHex(&ha1[0], &ha1_hex[0]);
  
  sprintf(result, "Digest username=\"%s\",realm=\"%s\"",user,tmp);
  if (cnonce[0] != '\0') {
    sprintf(result, "%s,cnonce=\"%s\",nc=%s,qop=%s",result,cnonce,nc,authtype);
  }
  
  // Construct the URI 

  sprintf(tmp, "%s", uri);
  
  // If using Auth-Int make a hash of the body - which is NULL for REG 
  if (stristr(authtype, "auth-int") != NULL) {
    MD5_Init(&Md5Ctx);
    MD5_Update(&Md5Ctx, msgbody, strlen(msgbody));
    MD5_Final(body, &Md5Ctx);
    hashToHex(&body[0], &body_hex[0]);
  }
  
  // Load in A2 
  MD5_Init(&Md5Ctx);
  MD5_Update(&Md5Ctx, method, strlen(method));
  MD5_Update(&Md5Ctx, ":", 1);
  MD5_Update(&Md5Ctx, tmp, strlen(tmp));
  if (stristr(authtype, "auth-int") != NULL) {
    MD5_Update(&Md5Ctx, ":", 1);
    MD5_Update(&Md5Ctx, &body_hex, HASH_HEX_SIZE);
  }
  MD5_Final(ha2, &Md5Ctx);
  hashToHex(&ha2[0], &ha2_hex[0]);
  
  sprintf(result, "%s,uri=\"%s\"",result,tmp);
  
  // Extract the Nonce 
  if ((start = stristr(auth, "nonce=")) == NULL) {
    sprintf(result, "createAuthHeader: couldn't parse nonce");
    return 0;
  }
  start = start + strlen("nonce=");
  if (*start == '"') { start++; }
  end = start + strcspn(start, " ,\"\r\n");
  strncpy(tmp, start, end - start);
  tmp[end - start] ='\0';
  
  MD5_Init(&Md5Ctx);
  MD5_Update(&Md5Ctx, &ha1_hex, HASH_HEX_SIZE);
  MD5_Update(&Md5Ctx, ":", 1);
  MD5_Update(&Md5Ctx, tmp, strlen(tmp));
  if (cnonce[0] != '\0') {
    MD5_Update(&Md5Ctx, ":", 1);
    MD5_Update(&Md5Ctx, nc, strlen(nc));
    MD5_Update(&Md5Ctx, ":", 1);
    MD5_Update(&Md5Ctx, cnonce, strlen(cnonce));
    MD5_Update(&Md5Ctx, ":", 1);
    MD5_Update(&Md5Ctx, authtype, strlen(authtype));
  }
  MD5_Update(&Md5Ctx, ":", 1);
  MD5_Update(&Md5Ctx, &ha2_hex, HASH_HEX_SIZE);
  MD5_Final(resp, &Md5Ctx);
  hashToHex(&resp[0], &resp_hex[0]);
  
  sprintf(result, "%s,nonce=\"%s\",response=\"%s\",algorithm=%s",result,tmp,resp_hex,algo);
  
  if (has_opaque) {
    sprintf(result, "%s,opaque=\"%s\"",result,opaque);
  }
  
  return 1;
}


static int base64_val(char x)\
{
	switch(x){
		case '=': return -1;
		case 'A': return 0;
		case 'B': return 1;
		case 'C': return 2;
		case 'D': return 3;
		case 'E': return 4;
		case 'F': return 5;
		case 'G': return 6;
		case 'H': return 7;
		case 'I': return 8;
		case 'J': return 9;
		case 'K': return 10;
		case 'L': return 11;
		case 'M': return 12;
		case 'N': return 13;
		case 'O': return 14;
		case 'P': return 15;
		case 'Q': return 16;
		case 'R': return 17;
		case 'S': return 18;
		case 'T': return 19;
		case 'U': return 20;
		case 'V': return 21;
		case 'W': return 22;
		case 'X': return 23;
		case 'Y': return 24;
		case 'Z': return 25;
		case 'a': return 26;
		case 'b': return 27;
		case 'c': return 28;
		case 'd': return 29;
		case 'e': return 30;
		case 'f': return 31;
		case 'g': return 32;
		case 'h': return 33;
		case 'i': return 34;
		case 'j': return 35;
		case 'k': return 36;
		case 'l': return 37;
		case 'm': return 38;
		case 'n': return 39;
		case 'o': return 40;
		case 'p': return 41;
		case 'q': return 42;
		case 'r': return 43;
		case 's': return 44;
		case 't': return 45;
		case 'u': return 46;
		case 'v': return 47;
		case 'w': return 48;
		case 'x': return 49;
		case 'y': return 50;
		case 'z': return 51;
		case '0': return 52;
		case '1': return 53;
		case '2': return 54;
		case '3': return 55;
		case '4': return 56;
		case '5': return 57;
		case '6': return 58;
		case '7': return 59;
		case '8': return 60;
		case '9': return 61;
		case '+': return 62;
		case '/': return 63;
	}
	return 0;
}

char * base64_decode_string( const char *buf, unsigned int len, int *newlen )
{
  unsigned int i ;
  int j,x1,x2,x3,x4;
  char *out;
  ALLOC_VAR(out,char *,(( len * 3/4 ) + 8 ));
  for(i=0,j=0;i+3<len;i+=4){
    x1=base64_val(buf[i]);
    x2=base64_val(buf[i+1]);
    x3=base64_val(buf[i+2]);
    x4=base64_val(buf[i+3]);
    out[j++]=(x1<<2) | ((x2 & 0x30)>>4);
    out[j++]=((x2 & 0x0F)<<4) | ((x3 & 0x3C)>>2);
    out[j++]=((x3 & 0x03)<<6) | (x4 & 0x3F);
  }
  if (i<len) {
    x1 = base64_val(buf[i]);
    if (i+1<len)
      x2=base64_val(buf[i+1]);
    else 
      x2=-1;
    if (i+2<len)		
      x3=base64_val(buf[i+2]);
    else
      x3=-1;
    if(i+3<len)	
      x4=base64_val(buf[i+3]);
    else x4=-1;
    if (x2!=-1) {
      out[j++]=(x1<<2) | ((x2 & 0x30)>>4);
      if (x3==-1) {
        out[j++]=((x2 & 0x0F)<<4) | ((x3 & 0x3C)>>2);
        if (x4==-1) {
          out[j++]=((x3 & 0x03)<<6) | (x4 & 0x3F);
        }
      }
    }
    
  }
  
  out[j++] = 0;
  *newlen=j;
  return out;
}


char base64[64]= {
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q',
  'R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h',
  'i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y',
  'z','0','1','2','3','4','5','6','7','8','9','+','/' } ;

char * base64_encode_string( const char *buf, unsigned int len, int *newlen )
{
  int i,k;
  int triplets,rest;
  char *out,*ptr;	
  
  triplets = len/3;
  rest = len%3;
  ALLOC_VAR(out,char *,(( triplets * 4 ) + 8 ));
  
  ptr = out;
  for(i=0;i<triplets*3;i+=3){
    k = (((unsigned char) buf[i])&0xFC)>>2;
    *ptr=base64[k];ptr++;
    
    k = (((unsigned char) buf[i])&0x03)<<4;
    k |=(((unsigned char) buf[i+1])&0xF0)>>4;
    *ptr=base64[k];ptr++;
    
    k = (((unsigned char) buf[i+1])&0x0F)<<2;
    k |=(((unsigned char) buf[i+2])&0xC0)>>6;
    *ptr=base64[k];ptr++;
    
    k = (((unsigned char) buf[i+2])&0x3F);
    *ptr=base64[k];ptr++;
  }
  i=triplets*3;
  switch(rest){
  case 0:
    break;
  case 1:
    k = (((unsigned char) buf[i])&0xFC)>>2;
    *ptr=base64[k];ptr++;
    
    k = (((unsigned char) buf[i])&0x03)<<4;
    *ptr=base64[k];ptr++;
    
    *ptr='=';ptr++;
    
    *ptr='=';ptr++;
    break;
  case 2:
    k = (((unsigned char) buf[i])&0xFC)>>2;
    *ptr=base64[k];ptr++;
    
    k = (((unsigned char) buf[i])&0x03)<<4;
    k |=(((unsigned char) buf[i+1])&0xF0)>>4;
    *ptr=base64[k];ptr++;
    
    k = (((unsigned char) buf[i+1])&0x0F)<<2;
    *ptr=base64[k];ptr++;
    
    *ptr='=';ptr++;
    break;
  }

  *newlen = ptr-out;
  return out;
}




char hexa[16]= { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' } ;

int createAuthHeaderAKAv1MD5(char * user, char * aka_OP, 
                             char * aka_AMF, 
                             char * aka_K, 
                             char * method,
                             char * uri, char * msgbody, char * auth, char *algo,
                             char * result) {
                     	
  char tmp[MAX_HEADER_LEN] ; 
  char *start, *end;
  int has_auts = 0, resuf = 1;
  char *nonce64, *nonce;
  int noncelen;
  RESHEX resp_hex;
  AMF amf;
  OP op;
  RAND rnd;
  AUTS auts_bin;
  AUTS64 auts_hex;
  MAC mac,xmac;
  SQN sqn, sqnxoraka, sqn_ms;
  K k;
  RES res;
  CK ck;
  IK ik;
  AK ak;
  int i;

  // Extract the Nonce 
  if ((start = stristr(auth, "nonce=")) == NULL) {
      sprintf(result, "createAuthHeaderAKAv1MD5: couldn't parse nonce");
      return 0;
  }
  start = start + strlen("nonce=");
  if (*start == '"') { start++; }
  end = start + strcspn(start, " ,\"\r\n");
  strncpy(tmp, start, end - start);
  tmp[end - start] ='\0';

  /* Compute the AKA RES */
  resp_hex[0]=0;
  nonce64 = tmp;
  nonce = base64_decode_string(nonce64,end-start,&noncelen);
  if (noncelen<RANDLEN+AUTNLEN) {
    sprintf(result,"createAuthHeaderAKAv1MD5 : Nonce is too short %d < %d expected \n",
    noncelen,RANDLEN+AUTNLEN);
    return 0;
  }
  memcpy(rnd,nonce,RANDLEN);
  memcpy(sqnxoraka,nonce+RANDLEN,SQNLEN);
  memcpy(mac,nonce+RANDLEN+SQNLEN+AMFLEN,MACLEN);
  memcpy(k,aka_K,KLEN);
  memcpy(amf,aka_AMF,AMFLEN);
  memcpy(op,aka_OP,OPLEN);

  /* Compute the AK, response and keys CK IK */
  f2345(k,rnd,res,ck,ik,ak,op);
  res[RESLEN]='\0';

  /* Compute sqn encoded in AUTN */
  for (i=0; i < SQNLEN; i++)
    sqn[i] = sqnxoraka[i] ^ ak[i];

  /* compute XMAC */
  f1(k,rnd,sqn,(unsigned char*)aka_AMF,xmac,op);
  if (memcmp(mac,xmac,MACLEN)!=0) {
    sprintf(result,"createAuthHeaderAKAv1MD5 : MAC != eXpectedMAC -> Server might not know the secret (man-in-the-middle attack?) \n");
    //return 0;
  }

  /* Check SQN, compute AUTS if needed and authorization parameter */
  /* the condition below is wrong.
   * Should trigger synchronization when sqn_ms>>3!=sqn_he>>3 for example.
   * Also, we need to store the SQN per user or put it as auth parameter. */
  if (1/*sqn[5] > sqn_he[5]*/) {
    sqn_he[5] = sqn[5];
    has_auts = 0;
    /* RES has to be used as password to compute response */
    for(i=0;i<RESLEN;i++){
      resp_hex[2*i]=hexa[(res[i]&0xF0)>>4];
      resp_hex[2*i+1]=hexa[res[i]&0x0F];
    }
    resp_hex[RESLEN*2]=0;
    resuf = createAuthHeaderMD5(user, resp_hex, method, uri, msgbody, auth, algo, result);   
  } else {
    sqn_ms[5] = sqn_he[5] + 1;
    f5star(k, rnd, ak, op);
    for(i=0; i<SQNLEN; i++)
      auts_bin[i]=sqn_ms[i]^ak[i];
    f1star(k, rnd, sqn_ms, amf, (unsigned char*)auts_bin+SQNLEN, op);
    has_auts = 1;
    /* When re-synchronisation occurs an empty password has to be used */
    /* to compute MD5 response (Cf. rfc 3310 section 3.2) */
    resuf=createAuthHeaderMD5(user,"",method,uri,msgbody,auth,algo,result);
  }
  if (has_auts) {
    /* Format data for output in the SIP message */
    for(i=0;i<AUTSLEN;i++){
      auts_hex[2*i]=hexa[(auts_bin[i]&0xF0)>>4];
      auts_hex[2*i+1]=hexa[auts_bin[i]&0x0F];
    }
    auts_hex[AUTS64LEN-1]=0;
    
    sprintf(result, "%s,auts=\"%s\"",result,auts_hex);
  }
  
  return 1;
}

