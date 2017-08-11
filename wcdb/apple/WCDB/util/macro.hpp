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

#ifndef macro_hpp
#define macro_hpp

//拼接ab
#define _CONCAT(a, b) a##b
//拼接ab
#define CONCAT(a, b) _CONCAT(a, b)
//__COUNTER__ 累加器，返回第一无二的id
#define UNUSED_UNIQUE_ID CONCAT(_unused, __COUNTER__)

#endif /* macro_hpp */
