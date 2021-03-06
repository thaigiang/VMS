/**
  ******************************************************************************
  * @file    rsa.h
  * @author  MCD Application Team
  * @version V2.0.6
  * @date    25-June-2013
  * @brief   Provides RSA operations with support for PKCS#1v1.5
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************/
/*!
* \page Tutorial_RSA RSA Tutorial
*
* This version (2.1) of the library supports RSA functions for signature generation/validation.
*
* Note that there is a configuration switch \ref RSA_WINDOW_SIZE that can be used to speedup
* operations with the private key.
*
* In order to pass the keys to the functions, there are two structures to be filled:
*  - \ref RSAprivKey_stt for the private key
*  - \ref RSApubKey_stt for the public key
*  The values of the byte arrays pointed by the above structures, as well as the signature \b must be
*  byte arrays where the byte at index 0 represents the most significant byte of the integer 
*  (modulus, signature or exponent). \n
*
*  All members of the above functions should be filled by the user prior to calls to the RSA functions,
*  which are:
*  - \ref RSA_PKCS1v15_Sign 
*  - \ref RSA_PKCS1v15_Verify
*
* Furthermore the user has to set up the \ref membuf_stt structure to point to a buffer of suitable
* size than will be used for the internal computations.
*
* A simple usage of the RSA PKCS#1v1.5 signature generation and verification is shown below:
*
* \code
* #include "rsa.h"
* int32_t main()
* {
*   uint8_t modulus[2048/8]={ ... };
*   uint8_t public_exponent[3]={0x01,0x00,0x01};
*   uint8_t digest[CRL_SHA256_SIZE]={...};
*   uint8_t signature[2048/8];
*   uint8_t private_exponent[2048/8]={...};
*   int32_t retval;
*   RSAprivKey_stt privKey;
*   RSApubKey_stt pubKey;
*   membuf_stt mb;  
*   uint8_t preallocated_buffer[4096];
*
*   //Set up the membuf_stt structure to a preallocated (on stack) buffer of 4kB
*   mb.mSize = sizeof(preallocated_buffer);
*   mb.mUsed = 0;
*   mb.pmBuf = preallocated_buffer;
*
*   // Set values of private key
*   privKey.mExponentSize = sizeof(private_exponent);
*   privKey.pmExponent = private_exponent;
*   privKey.mModulusSize = sizeof(modulus);
*   privKey.pmModulus = modulus;
*
*   //Generate the signature, knowing that the hash has been generated by SHA-256
*   retval = RSA_PKCS1v15_Sign(&privKey, digest, E_SHA256, signature, &mb);
*   if (retval != RSA_SUCCESS)
*   { return(ERROR); }
*
*   // Set values of public key
*   pubKey.mExponentSize = sizeof(public_exponent);
*   pubKey.pmExponent = public_exponent;
*   pubKey.mModulusSize = sizeof(modulus);
*   pubKey.pmModulus = modulus;
*
*   //Verify the signature, knowing that the hash has been generated by SHA-256
*   retval = RSA_PKCS1v15_Verify(&pubKey, digest, E_SHA256, signature, &mb)
*   if (retval != SIGNATURE_VALID )
*   { return(ERROR); }
*   else
*   { return(OK); }
* }
* \endcode
*
*/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CRL_RSA_H__
#define __CRL_RSA_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/** @ingroup RSA
  * @{
  */

/** 
  * @brief  Structure type for RSA public key 
  */   
typedef struct 
{ 
  uint8_t  *pmModulus;    /*!< RSA Modulus */ 
  int32_t  mModulusSize;  /*!< Size of RSA Modulus */  
  uint8_t  *pmExponent;   /*!< RSA Public Exponent */  
  int32_t  mExponentSize; /*!< Size of RSA Public Exponent */
} RSApubKey_stt;
  
/** 
  * @brief  Structure type for RSA private key
  */ 
typedef struct 
{  
  uint8_t  *pmModulus; /*!< RSA Modulus */
  int32_t  mModulusSize; /*!< Size of RSA Modulus */  
  uint8_t  *pmExponent; /*!< RSA Private Exponent */  
  int32_t  mExponentSize; /*!< Size of RSA Private Exponent */
} RSAprivKey_stt;

typedef struct
{
  const uint8_t *pmInput;       /*!< Pointer to input buffer */
  int32_t mInputSize; /*!< Size of input buffer */
  uint8_t *pmOutput;      /*!< Pointer to output buffer */
} RSAinOut_stt;


int32_t RSA_PKCS1v15_Sign(const RSAprivKey_stt *P_pPrivKey, const uint8_t *P_pDigest, hashType_et P_hashType, uint8_t *P_pSignature, membuf_stt *P_pMemBuf);

int32_t RSA_PKCS1v15_Verify(const RSApubKey_stt *P_pPubKey, const uint8_t *P_pDigest, hashType_et P_hashType, const uint8_t *P_pSignature, membuf_stt *P_pMemBuf);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __CRL_RSA_H__ */


/******************* (C) COPYRIGHT 2013 STMicroelectronics *****END OF FILE****/
