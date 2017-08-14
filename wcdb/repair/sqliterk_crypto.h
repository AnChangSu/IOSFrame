/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef sqliterk_crypto_h
#define sqliterk_crypto_h

#ifdef __cplusplus
extern "C" {
#endif
    
//kdf : 基于SHA-256的单步密钥导出函数
//kdf salt
/*
 Beware that you need to communicate the salt value if you use one in HKDF. For a KDF after Key Agreement - which should already provide you with enough entropy in the secret - I would consider it strictly optional.
 */

//数据库文件结构体
typedef struct sqliterk_file sqliterk_file;
//sqliterk_pager 管理单一数据库所有页的结构体
typedef struct sqliterk_pager sqliterk_pager;
//数据库加密上下文
typedef struct codec_ctx sqliterk_codec;
//数据库加密信息结构体
typedef struct sqliterk_cipher_conf sqliterk_cipher_conf;

//数据库加密设置加密信息
int sqliterkCryptoSetCipher(sqliterk_pager *pager,
                            sqliterk_file *fd,
                            const sqliterk_cipher_conf *conf);
//加密对象释页管理者
void sqliterkCryptoFreeCodec(sqliterk_pager *pager);
//解密指定页
int sqliterkCryptoDecode(sqliterk_codec *codec, int pgno, void *data);

#ifdef __cplusplus
}
#endif

#endif
