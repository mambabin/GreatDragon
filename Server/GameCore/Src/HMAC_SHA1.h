/*
	100% free public domain implementation of the HMAC-SHA1 algorithm
	by Chien-Chung, Chung (Jim Chung) <jimchung1221@gmail.com>
*/


#ifndef __HMAC_SHA1_H__
#define __HMAC_SHA1_H__

#include "SHA1.h"

typedef unsigned char BYTE ;

class CHMAC_SHA1 : public CSHA1
{

	public:
		enum {
			SHA1_DIGEST_LENGTH	= 20,
			SHA1_BLOCK_SIZE		= 64,
			HMAC_BUF_LEN		= 4096
		} ;

		CHMAC_SHA1()
			/*
			:szReport(new char[HMAC_BUF_LEN]),
             AppendBuf1(new char[HMAC_BUF_LEN]),
             AppendBuf2(new char[HMAC_BUF_LEN]),
             SHA1_Key(new char[HMAC_BUF_LEN])
			 */
		{}

        ~CHMAC_SHA1()
        {
			/*
            delete[] szReport ;
            delete[] AppendBuf1 ;
            delete[] AppendBuf2 ;
            delete[] SHA1_Key ;
			*/
        }

        void HMAC_SHA1(BYTE *text, int text_len, BYTE *key, int key_len, BYTE *digest);

    private:
		BYTE m_ipad[64];
        BYTE m_opad[64];

		char szReport[HMAC_BUF_LEN] ;
		char SHA1_Key[HMAC_BUF_LEN] ;
		char AppendBuf1[HMAC_BUF_LEN] ;
		char AppendBuf2[HMAC_BUF_LEN] ;
};


#endif /* __HMAC_SHA1_H__ */