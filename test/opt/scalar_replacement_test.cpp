// Copyright (c) 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "assembly_builder.h"
#include "gmock/gmock.h"
#include "pass_fixture.h"
#include "pass_utils.h"

namespace {

using namespace spvtools;

using ScalarReplacementTest = PassTest<::testing::Test>;

// TODO(dneto): Add Effcee as required dependency, and make this unconditional.
#ifdef SPIRV_EFFCEE
TEST_F(ScalarReplacementTest, SimpleStruct) {
  const std::string text = R"(
;
; CHECK: [[struct:%\w+]] = OpTypeStruct [[elem:%\w+]]
; CHECK: [[struct_ptr:%\w+]] = OpTypePointer Function [[struct]]
; CHECK: [[elem_ptr:%\w+]] = OpTypePointer Function [[elem]]
; CHECK: OpConstantNull [[struct]]
; CHECK: [[null:%\w+]] = OpConstantNull [[elem]]
; CHECK-NOT: OpVariable [[struct_ptr]]
; CHECK: [[one:%\w+]] = OpVariable [[elem_ptr]] Function [[null]]
; CHECK-NEXT: [[two:%\w+]] = OpVariable [[elem_ptr]] Function [[null]]
; CHECK-NOT: OpVariable [[elem_ptr]] Function [[null]]
; CHECK-NOT: OpVariable [[struct_ptr]]
; CHECK-NOT: OpInBoundsAccessChain
; CHECK: [[l1:%\w+]] = OpLoad [[elem]] [[two]]
; CHECK-NOT: OpAccessChain
; CHECK: [[l2:%\w+]] = OpLoad [[elem]] [[one]]
; CHECK: OpIAdd [[elem]] [[l1]] [[l2]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %6 "simple_struct"
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeStruct %2 %2 %2 %2
%4 = OpTypePointer Function %3
%5 = OpTypePointer Function %2
%6 = OpTypeFunction %2
%7 = OpConstantNull %3
%8 = OpConstant %2 0
%9 = OpConstant %2 1
%10 = OpConstant %2 2
%11 = OpConstant %2 3
%12 = OpFunction %2 None %6
%13 = OpLabel
%14 = OpVariable %4 Function %7
%15 = OpInBoundsAccessChain %5 %14 %8
%16 = OpLoad %2 %15
%17 = OpAccessChain %5 %14 %10
%18 = OpLoad %2 %17
%19 = OpIAdd %2 %16 %18
OpReturnValue %19
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, StructInitialization) {
  const std::string text = R"(
;
; CHECK: [[elem:%\w+]] = OpTypeInt 32 0
; CHECK: [[struct:%\w+]] = OpTypeStruct [[elem]] [[elem]] [[elem]] [[elem]]
; CHECK: [[struct_ptr:%\w+]] = OpTypePointer Function [[struct]]
; CHECK: [[elem_ptr:%\w+]] = OpTypePointer Function [[elem]]
; CHECK: [[zero:%\w+]] = OpConstant [[elem]] 0
; CHECK: [[undef:%\w+]] = OpUndef [[elem]]
; CHECK: [[two:%\w+]] = OpConstant [[elem]] 2
; CHECK: [[null:%\w+]] = OpConstantNull [[elem]]
; CHECK-NOT: OpVariable [[struct_ptr]]
; CHECK: OpVariable [[elem_ptr]] Function [[null]]
; CHECK-NEXT: OpVariable [[elem_ptr]] Function [[two]]
; CHECK-NOT: OpVariable [[elem_ptr]] Function [[undef]]
; CHECK-NEXT: OpVariable [[elem_ptr]] Function
; CHECK-NEXT: OpVariable [[elem_ptr]] Function [[zero]]
; CHECK-NOT: OpVariable [[elem_ptr]] Function [[undef]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %6 "struct_init"
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeStruct %2 %2 %2 %2
%4 = OpTypePointer Function %3
%20 = OpTypePointer Function %2
%6 = OpTypeFunction %1
%7 = OpConstant %2 0
%8 = OpUndef %2
%9 = OpConstant %2 2
%30 = OpConstant %2 1
%31 = OpConstant %2 3
%10 = OpConstantNull %2
%11 = OpConstantComposite %3 %7 %8 %9 %10
%12 = OpFunction %1 None %6
%13 = OpLabel
%14 = OpVariable %4 Function %11
%15 = OpAccessChain %20 %14 %7
OpStore %15 %10
%16 = OpAccessChain %20 %14 %9
OpStore %16 %10
%17 = OpAccessChain %20 %14 %30
OpStore %17 %10
%18 = OpAccessChain %20 %14 %31
OpStore %18 %10
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, SpecConstantInitialization) {
  const std::string text = R"(
;
; CHECK: [[int:%\w+]] = OpTypeInt 32 0
; CHECK: [[struct:%\w+]] = OpTypeStruct [[int]] [[int]]
; CHECK: [[struct_ptr:%\w+]] = OpTypePointer Function [[struct]]
; CHECK: [[int_ptr:%\w+]] = OpTypePointer Function [[int]]
; CHECK: [[spec_comp:%\w+]] = OpSpecConstantComposite [[struct]]
; CHECK: [[ex0:%\w+]] = OpSpecConstantOp [[int]] CompositeExtract [[spec_comp]] 0
; CHECK: [[ex1:%\w+]] = OpSpecConstantOp [[int]] CompositeExtract [[spec_comp]] 1
; CHECK-NOT: OpVariable [[struct]]
; CHECK: OpVariable [[int_ptr]] Function [[ex1]]
; CHECK-NEXT: OpVariable [[int_ptr]] Function [[ex0]]
; CHECK-NOT: OpVariable [[struct]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %6 "spec_const"
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeStruct %2 %2
%4 = OpTypePointer Function %3
%20 = OpTypePointer Function %2
%5 = OpTypeFunction %1
%6 = OpConstant %2 0
%30 = OpConstant %2 1
%7 = OpSpecConstant %2 0
%8 = OpSpecConstantOp %2 IAdd %7 %7
%9 = OpSpecConstantComposite %3 %7 %8
%10 = OpFunction %1 None %5
%11 = OpLabel
%12 = OpVariable %4 Function %9
%13 = OpAccessChain %20 %12 %6
%14 = OpLoad %2 %13
%15 = OpAccessChain %20 %12 %30
%16 = OpLoad %2 %15
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

// TODO(alanbaker): Re-enable when vector and matrix scalarization is supported.
// TEST_F(ScalarReplacementTest, VectorInitialization) {
//  const std::string text = R"(
//;
//; CHECK: [[elem:%\w+]] = OpTypeInt 32 0
//; CHECK: [[vector:%\w+]] = OpTypeVector [[elem]] 4
//; CHECK: [[vector_ptr:%\w+]] = OpTypePointer Function [[vector]]
//; CHECK: [[elem_ptr:%\w+]] = OpTypePointer Function [[elem]]
//; CHECK: [[zero:%\w+]] = OpConstant [[elem]] 0
//; CHECK: [[undef:%\w+]] = OpUndef [[elem]]
//; CHECK: [[two:%\w+]] = OpConstant [[elem]] 2
//; CHECK: [[null:%\w+]] = OpConstantNull [[elem]]
//; CHECK-NOT: OpVariable [[vector_ptr]]
//; CHECK: OpVariable [[elem_ptr]] Function [[zero]]
//; CHECK-NOT: OpVariable [[elem_ptr]] Function [[undef]]
//; CHECK-NEXT: OpVariable [[elem_ptr]] Function
//; CHECK-NEXT: OpVariable [[elem_ptr]] Function [[two]]
//; CHECK-NEXT: OpVariable [[elem_ptr]] Function [[null]]
//; CHECK-NOT: OpVariable [[elem_ptr]] Function [[undef]]
//;
// OpCapability Shader
// OpCapability Linkage
// OpMemoryModel Logical GLSL450
// OpName %6 "vector_init"
//%1 = OpTypeVoid
//%2 = OpTypeInt 32 0
//%3 = OpTypeVector %2 4
//%4 = OpTypePointer Function %3
//%20 = OpTypePointer Function %2
//%6 = OpTypeFunction %1
//%7 = OpConstant %2 0
//%8 = OpUndef %2
//%9 = OpConstant %2 2
//%30 = OpConstant %2 1
//%31 = OpConstant %2 3
//%10 = OpConstantNull %2
//%11 = OpConstantComposite %3 %10 %9 %8 %7
//%12 = OpFunction %1 None %6
//%13 = OpLabel
//%14 = OpVariable %4 Function %11
//%15 = OpAccessChain %20 %14 %7
// OpStore %15 %10
//%16 = OpAccessChain %20 %14 %9
// OpStore %16 %10
//%17 = OpAccessChain %20 %14 %30
// OpStore %17 %10
//%18 = OpAccessChain %20 %14 %31
// OpStore %18 %10
// OpReturn
// OpFunctionEnd
//  )";
//
//  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
//}
//
// TEST_F(ScalarReplacementTest, MatrixInitialization) {
//  const std::string text = R"(
//;
//; CHECK: [[float:%\w+]] = OpTypeFloat 32
//; CHECK: [[vector:%\w+]] = OpTypeVector [[float]] 2
//; CHECK: [[matrix:%\w+]] = OpTypeMatrix [[vector]] 2
//; CHECK: [[matrix_ptr:%\w+]] = OpTypePointer Function [[matrix]]
//; CHECK: [[float_ptr:%\w+]] = OpTypePointer Function [[float]]
//; CHECK: [[vec_ptr:%\w+]] = OpTypePointer Function [[vector]]
//; CHECK: [[zerof:%\w+]] = OpConstant [[float]] 0
//; CHECK: [[onef:%\w+]] = OpConstant [[float]] 1
//; CHECK: [[one_zero:%\w+]] = OpConstantComposite [[vector]] [[onef]] [[zerof]]
//; CHECK: [[zero_one:%\w+]] = OpConstantComposite [[vector]] [[zerof]] [[onef]]
//; CHECK: [[const_mat:%\w+]] = OpConstantComposite [[matrix]] [[one_zero]]
//[[zero_one]] ; CHECK-NOT: OpVariable [[matrix]] ; CHECK-NOT: OpVariable
//[[vector]] Function [[one_zero]] ; CHECK: [[f1:%\w+]] = OpVariable
//[[float_ptr]] Function [[zerof]] ; CHECK-NEXT: [[f2:%\w+]] = OpVariable
//[[float_ptr]] Function [[onef]] ; CHECK-NEXT: [[vec_var:%\w+]] = OpVariable
//[[vec_ptr]] Function [[zero_one]] ; CHECK-NOT: OpVariable [[matrix]] ;
// CHECK-NOT: OpVariable [[vector]] Function [[one_zero]]
//;
// OpCapability Shader
// OpCapability Linkage
// OpMemoryModel Logical GLSL450
// OpName %7 "matrix_init"
//%1 = OpTypeVoid
//%2 = OpTypeFloat 32
//%3 = OpTypeVector %2 2
//%4 = OpTypeMatrix %3 2
//%5 = OpTypePointer Function %4
//%6 = OpTypePointer Function %2
//%30 = OpTypePointer Function %3
//%10 = OpTypeInt 32 0
//%7 = OpTypeFunction %1 %10
//%8 = OpConstant %2 0.0
//%9 = OpConstant %2 1.0
//%11 = OpConstant %10 0
//%12 = OpConstant %10 1
//%13 = OpConstantComposite %3 %9 %8
//%14 = OpConstantComposite %3 %8 %9
//%15 = OpConstantComposite %4 %13 %14
//%16 = OpFunction %1 None %7
//%31 = OpFunctionParameter %10
//%17 = OpLabel
//%18 = OpVariable %5 Function %15
//%19 = OpAccessChain %6 %18 %11 %12
// OpStore %19 %8
//%20 = OpAccessChain %6 %18 %11 %11
// OpStore %20 %8
//%21 = OpAccessChain %30 %18 %12
// OpStore %21 %14
// OpReturn
// OpFunctionEnd
//  )";
//
//  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
//}

TEST_F(ScalarReplacementTest, ElideAccessChain) {
  const std::string text = R"(
;
; CHECK: [[var:%\w+]] = OpVariable
; CHECK-NOT: OpAccessChain
; CHECK: OpStore [[var]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %6 "elide_access_chain"
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeStruct %2 %2 %2 %2
%4 = OpTypePointer Function %3
%20 = OpTypePointer Function %2
%6 = OpTypeFunction %1
%7 = OpConstant %2 0
%8 = OpUndef %2
%9 = OpConstant %2 2
%10 = OpConstantNull %2
%11 = OpConstantComposite %3 %7 %8 %9 %10
%12 = OpFunction %1 None %6
%13 = OpLabel
%14 = OpVariable %4 Function %11
%15 = OpAccessChain %20 %14 %7
OpStore %15 %10
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, ElideMultipleAccessChains) {
  const std::string text = R"(
;
; CHECK: [[var:%\w+]] = OpVariable
; CHECK-NOT: OpInBoundsAccessChain
; CHECK OpStore [[var]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %6 "elide_two_access_chains"
%1 = OpTypeVoid
%2 = OpTypeFloat 32
%3 = OpTypeStruct %2 %2
%4 = OpTypeStruct %3 %3
%5 = OpTypePointer Function %4
%6 = OpTypePointer Function %2
%7 = OpTypeFunction %1
%8 = OpConstant %2 0.0
%9 = OpConstant %2 1.0
%10 = OpTypeInt 32 0
%11 = OpConstant %10 0
%12 = OpConstant %10 1
%13 = OpConstantComposite %3 %9 %8
%14 = OpConstantComposite %3 %8 %9
%15 = OpConstantComposite %4 %13 %14
%16 = OpFunction %1 None %7
%17 = OpLabel
%18 = OpVariable %5 Function %15
%19 = OpInBoundsAccessChain %6 %18 %11 %12
OpStore %19 %8
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, ReplaceAccessChain) {
  const std::string text = R"(
;
; CHECK: [[param:%\w+]] = OpFunctionParameter
; CHECK: [[var:%\w+]] = OpVariable
; CHECK: [[access:%\w+]] = OpAccessChain {{%\w+}} [[var]] [[param]]
; CHECK: OpStore [[access]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %7 "replace_access_chain"
%1 = OpTypeVoid
%2 = OpTypeFloat 32
%10 = OpTypeInt 32 0
%uint_2 = OpConstant %10 2
%3 = OpTypeArray %2 %uint_2
%4 = OpTypeStruct %3 %3
%5 = OpTypePointer Function %4
%20 = OpTypePointer Function %3
%6 = OpTypePointer Function %2
%7 = OpTypeFunction %1 %10
%8 = OpConstant %2 0.0
%9 = OpConstant %2 1.0
%11 = OpConstant %10 0
%12 = OpConstant %10 1
%13 = OpConstantComposite %3 %9 %8
%14 = OpConstantComposite %3 %8 %9
%15 = OpConstantComposite %4 %13 %14
%16 = OpFunction %1 None %7
%32 = OpFunctionParameter %10
%17 = OpLabel
%18 = OpVariable %5 Function %15
%19 = OpAccessChain %6 %18 %11 %32
OpStore %19 %8
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, ArrayInitialization) {
  const std::string text = R"(
;
; CHECK: [[float:%\w+]] = OpTypeFloat 32
; CHECK: [[array:%\w+]] = OpTypeArray
; CHECK: [[array_ptr:%\w+]] = OpTypePointer Function [[array]]
; CHECK: [[float_ptr:%\w+]] = OpTypePointer Function [[float]]
; CHECK: [[float0:%\w+]] = OpConstant [[float]] 0
; CHECK: [[float1:%\w+]] = OpConstant [[float]] 1
; CHECK: [[float2:%\w+]] = OpConstant [[float]] 2
; CHECK-NOT: OpVariable [[array_ptr]]
; CHECK: [[var0:%\w+]] = OpVariable [[float_ptr]] Function [[float0]]
; CHECK-NEXT: [[var1:%\w+]] = OpVariable [[float_ptr]] Function [[float1]]
; CHECK-NEXT: [[var2:%\w+]] = OpVariable [[float_ptr]] Function [[float2]]
; CHECK-NOT: OpVariable [[array_ptr]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "array_init"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%float = OpTypeFloat 32
%uint_0 = OpConstant %uint 0
%uint_1 = OpConstant %uint 1
%uint_2 = OpConstant %uint 2
%uint_3 = OpConstant %uint 3
%float_array = OpTypeArray %float %uint_3
%array_ptr = OpTypePointer Function %float_array
%float_ptr = OpTypePointer Function %float
%float_0 = OpConstant %float 0
%float_1 = OpConstant %float 1
%float_2 = OpConstant %float 2
%const_array = OpConstantComposite %float_array %float_2 %float_1 %float_0
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%3 = OpVariable %array_ptr Function %const_array
%4 = OpInBoundsAccessChain %float_ptr %3 %uint_0
OpStore %4 %float_0
%5 = OpInBoundsAccessChain %float_ptr %3 %uint_1
OpStore %5 %float_0
%6 = OpInBoundsAccessChain %float_ptr %3 %uint_2
OpStore %6 %float_0
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
  ;
}

TEST_F(ScalarReplacementTest, NonUniformCompositeInitialization) {
  const std::string text = R"(
;
; CHECK: [[uint:%\w+]] = OpTypeInt 32 0
; CHECK: [[array:%\w+]] = OpTypeArray
; CHECK: [[matrix:%\w+]] = OpTypeMatrix
; CHECK: [[struct1:%\w+]] = OpTypeStruct [[uint]]
; CHECK: [[struct2:%\w+]] = OpTypeStruct [[struct1]] [[matrix]] [[array]] [[uint]]
; CHECK: [[struct1_ptr:%\w+]] = OpTypePointer Function [[struct1]]
; CHECK: [[matrix_ptr:%\w+]] = OpTypePointer Function [[matrix]]
; CHECK: [[array_ptr:%\w+]] = OpTypePointer Function [[array]]
; CHECK: [[uint_ptr:%\w+]] = OpTypePointer Function [[uint]]
; CHECK: [[struct2_ptr:%\w+]] = OpTypePointer Function [[struct2]]
; CHECK: [[const_uint:%\w+]] = OpConstant [[uint]]
; CHECK: [[const_array:%\w+]] = OpConstantComposite [[array]]
; CHECK: [[const_matrix:%\w+]] = OpConstantNull [[matrix]]
; CHECK: [[const_struct1:%\w+]] = OpConstantComposite [[struct1]]
; CHECK-NOT: OpVariable [[struct2_ptr]] Function
; CHECK: OpVariable [[uint_ptr]] Function [[const_uint]]
; CHECK-NEXT: OpVariable [[array_ptr]] Function [[const_array]]
; CHECK-NEXT: OpVariable [[matrix_ptr]] Function [[const_matrix]]
; CHECK-NEXT: OpVariable [[struct1_ptr]] Function [[const_struct1]]
; CHECK-NOT: OpVariable [[struct2_ptr]] Function
;
OpCapability Shader
OpCapability Linkage
OpCapability Int64
OpCapability Float64
OpMemoryModel Logical GLSL450
OpName %func "non_uniform_composite_init"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%int64 = OpTypeInt 64 1
%float = OpTypeFloat 32
%double = OpTypeFloat 64
%double2 = OpTypeVector %double 2
%float4 = OpTypeVector %float 4
%int64_0 = OpConstant %int64 0
%int64_1 = OpConstant %int64 1
%int64_2 = OpConstant %int64 2
%int64_3 = OpConstant %int64 3
%int64_array3 = OpTypeArray %int64 %int64_3
%matrix_double2 = OpTypeMatrix %double2 2
%struct1 = OpTypeStruct %uint %float4
%struct2 = OpTypeStruct %struct1 %matrix_double2 %int64_array3 %uint
%struct1_ptr = OpTypePointer Function %struct1
%matrix_double2_ptr = OpTypePointer Function %matrix_double2
%int64_array_ptr = OpTypePointer Function %int64_array3
%uint_ptr = OpTypePointer Function %uint
%struct2_ptr = OpTypePointer Function %struct2
%const_uint = OpConstant %uint 0
%const_int64_array = OpConstantComposite %int64_array3 %int64_0 %int64_1 %int64_2
%const_double2 = OpConstantNull %double2
%const_matrix_double2 = OpConstantNull %matrix_double2
%undef_float4 = OpUndef %float4
%const_struct1 = OpConstantComposite %struct1 %const_uint %undef_float4
%const_struct2 = OpConstantComposite %struct2 %const_struct1 %const_matrix_double2 %const_int64_array %const_uint
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct2_ptr Function %const_struct2
%3 = OpAccessChain %struct1_ptr %var %int64_0
OpStore %3 %const_struct1
%4 = OpAccessChain %matrix_double2_ptr %var %int64_1
OpStore %4 %const_matrix_double2
%5 = OpAccessChain %int64_array_ptr %var %int64_2
OpStore %5 %const_int64_array
%6 = OpAccessChain %uint_ptr %var %int64_3
OpStore %6 %const_uint
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
  ;
}

TEST_F(ScalarReplacementTest, ElideUncombinedAccessChains) {
  const std::string text = R"(
;
; CHECK: [[uint:%\w+]] = OpTypeInt 32 0
; CHECK: [[uint_ptr:%\w+]] = OpTypePointer Function [[uint]]
; CHECK: [[const:%\w+]] = OpConstant [[uint]] 0
; CHECK: [[var:%\w+]] = OpVariable [[uint_ptr]] Function
; CHECK-NOT: OpAccessChain
; CHECK: OpStore [[var]] [[const]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "elide_uncombined_access_chains"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint
%struct2 = OpTypeStruct %struct1
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%struct2_ptr = OpTypePointer Function %struct2
%uint_0 = OpConstant %uint 0
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct2_ptr Function
%3 = OpAccessChain %struct1_ptr %var %uint_0
%4 = OpAccessChain %uint_ptr %3 %uint_0
OpStore %4 %uint_0
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, ElideSingleUncombinedAccessChains) {
  const std::string text = R"(
;
; CHECK: [[uint:%\w+]] = OpTypeInt 32 0
; CHECK: [[array:%\w+]] = OpTypeArray [[uint]]
; CHECK: [[array_ptr:%\w+]] = OpTypePointer Function [[array]]
; CHECK: [[const:%\w+]] = OpConstant [[uint]] 0
; CHECK: [[param:%\w+]] = OpFunctionParameter [[uint]]
; CHECK: [[var:%\w+]] = OpVariable [[array_ptr]] Function
; CHECK: [[access:%\w+]] = OpAccessChain {{.*}} [[var]] [[param]]
; CHECK: OpStore [[access]] [[const]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "elide_single_uncombined_access_chains"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%uint_1 = OpConstant %uint 1
%array = OpTypeArray %uint %uint_1
%struct2 = OpTypeStruct %array
%uint_ptr = OpTypePointer Function %uint
%array_ptr = OpTypePointer Function %array
%struct2_ptr = OpTypePointer Function %struct2
%uint_0 = OpConstant %uint 0
%func = OpTypeFunction %void %uint
%1 = OpFunction %void None %func
%param = OpFunctionParameter %uint
%2 = OpLabel
%var = OpVariable %struct2_ptr Function
%3 = OpAccessChain %array_ptr %var %uint_0
%4 = OpAccessChain %uint_ptr %3 %param
OpStore %4 %uint_0
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, ReplaceWholeLoad) {
  const std::string text = R"(
;
; CHECK: [[uint:%\w+]] = OpTypeInt 32 0
; CHECK: [[struct1:%\w+]] = OpTypeStruct [[uint]] [[uint]]
; CHECK: [[uint_ptr:%\w+]] = OpTypePointer Function [[uint]]
; CHECK: [[const:%\w+]] = OpConstant [[uint]] 0
; CHECK: [[var1:%\w+]] = OpVariable [[uint_ptr]] Function
; CHECK: [[var0:%\w+]] = OpVariable [[uint_ptr]] Function
; CHECK: [[l1:%\w+]] = OpLoad [[uint]] [[var1]]
; CHECK: [[l0:%\w+]] = OpLoad [[uint]] [[var0]]
; CHECK: OpCompositeConstruct [[struct1]] [[l0]] [[l1]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "replace_whole_load"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint %uint
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%uint_0 = OpConstant %uint 0
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
%load = OpLoad %struct1 %var
%3 = OpAccessChain %uint_ptr %var %uint_0
OpStore %3 %uint_0
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, ReplaceWholeLoadCopyMemoryAccess) {
  const std::string text = R"(
;
; CHECK: [[uint:%\w+]] = OpTypeInt 32 0
; CHECK: [[struct1:%\w+]] = OpTypeStruct [[uint]] [[uint]]
; CHECK: [[uint_ptr:%\w+]] = OpTypePointer Function [[uint]]
; CHECK: [[const:%\w+]] = OpConstant [[uint]] 0
; CHECK: [[var1:%\w+]] = OpVariable [[uint_ptr]] Function
; CHECK: [[var0:%\w+]] = OpVariable [[uint_ptr]] Function
; CHECK: [[l1:%\w+]] = OpLoad [[uint]] [[var1]] Nontemporal
; CHECK: [[l0:%\w+]] = OpLoad [[uint]] [[var0]] Nontemporal
; CHECK: OpCompositeConstruct [[struct1]] [[l0]] [[l1]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "replace_whole_load_copy_memory_access"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint %uint
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%uint_0 = OpConstant %uint 0
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
%load = OpLoad %struct1 %var Nontemporal
%3 = OpAccessChain %uint_ptr %var %uint_0
OpStore %3 %uint_0
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, ReplaceWholeStore) {
  const std::string text = R"(
;
; CHECK: [[uint:%\w+]] = OpTypeInt 32 0
; CHECK: [[struct1:%\w+]] = OpTypeStruct [[uint]] [[uint]]
; CHECK: [[uint_ptr:%\w+]] = OpTypePointer Function [[uint]]
; CHECK: [[const:%\w+]] = OpConstant [[uint]] 0
; CHECK: [[const_struct:%\w+]] = OpConstantComposite [[struct1]] [[const]] [[const]]
; CHECK: [[var1:%\w+]] = OpVariable [[uint_ptr]] Function
; CHECK: [[var0:%\w+]] = OpVariable [[uint_ptr]] Function
; CHECK: [[ex0:%\w+]] = OpCompositeExtract [[uint]] [[const_struct]] 0
; CHECK: OpStore [[var0]] [[ex0]]
; CHECK: [[ex1:%\w+]] = OpCompositeExtract [[uint]] [[const_struct]] 1
; CHECK: OpStore [[var1]] [[ex1]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "replace_whole_store"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint %uint
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%uint_0 = OpConstant %uint 0
%const_struct = OpConstantComposite %struct1 %uint_0 %uint_0
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
OpStore %var %const_struct
%3 = OpAccessChain %uint_ptr %var %uint_0
%4 = OpLoad %uint %3
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, ReplaceWholeStoreCopyMemoryAccess) {
  const std::string text = R"(
;
; CHECK: [[uint:%\w+]] = OpTypeInt 32 0
; CHECK: [[struct1:%\w+]] = OpTypeStruct [[uint]] [[uint]]
; CHECK: [[uint_ptr:%\w+]] = OpTypePointer Function [[uint]]
; CHECK: [[const:%\w+]] = OpConstant [[uint]] 0
; CHECK: [[const_struct:%\w+]] = OpConstantComposite [[struct1]] [[const]] [[const]]
; CHECK: [[var1:%\w+]] = OpVariable [[uint_ptr]] Function
; CHECK: [[var0:%\w+]] = OpVariable [[uint_ptr]] Function
; CHECK: [[ex0:%\w+]] = OpCompositeExtract [[uint]] [[const_struct]] 0
; CHECK: OpStore [[var0]] [[ex0]] Aligned 4
; CHECK: [[ex1:%\w+]] = OpCompositeExtract [[uint]] [[const_struct]] 1
; CHECK: OpStore [[var1]] [[ex1]] Aligned 4
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "replace_whole_store_copy_memory_access"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint %uint
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%uint_0 = OpConstant %uint 0
%const_struct = OpConstantComposite %struct1 %uint_0 %uint_0
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
OpStore %var %const_struct Aligned 4
%3 = OpAccessChain %uint_ptr %var %uint_0
%4 = OpLoad %uint %3
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, DontTouchVolatileLoad) {
  const std::string text = R"(
;
; CHECK: [[struct:%\w+]] = OpTypeStruct
; CHECK: [[struct_ptr:%\w+]] = OpTypePointer Function [[struct]]
; CHECK: OpLabel
; CHECK-NEXT: OpVariable [[struct_ptr]]
; CHECK-NOT: OpVariable
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "dont_touch_volatile_load"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%uint_0 = OpConstant %uint 0
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
%3 = OpAccessChain %uint_ptr %var %uint_0
%4 = OpLoad %uint %3 Volatile
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, DontTouchVolatileStore) {
  const std::string text = R"(
;
; CHECK: [[struct:%\w+]] = OpTypeStruct
; CHECK: [[struct_ptr:%\w+]] = OpTypePointer Function [[struct]]
; CHECK: OpLabel
; CHECK-NEXT: OpVariable [[struct_ptr]]
; CHECK-NOT: OpVariable
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "dont_touch_volatile_store"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%uint_0 = OpConstant %uint 0
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
%3 = OpAccessChain %uint_ptr %var %uint_0
OpStore %3 %uint_0 Volatile
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, DontTouchSpecNonFunctionVariable) {
  const std::string text = R"(
;
; CHECK: [[struct:%\w+]] = OpTypeStruct
; CHECK: [[struct_ptr:%\w+]] = OpTypePointer Uniform [[struct]]
; CHECK: OpConstant
; CHECK-NEXT: OpVariable [[struct_ptr]]
; CHECK-NOT: OpVariable
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "dont_touch_spec_constant_access_chain"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint
%uint_ptr = OpTypePointer Uniform %uint
%struct1_ptr = OpTypePointer Uniform %struct1
%uint_0 = OpConstant %uint 0
%var = OpVariable %struct1_ptr Uniform
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%3 = OpAccessChain %uint_ptr %var %uint_0
OpStore %3 %uint_0 Volatile
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, DontTouchSpecConstantAccessChain) {
  const std::string text = R"(
;
; CHECK: [[array:%\w+]] = OpTypeArray
; CHECK: [[array_ptr:%\w+]] = OpTypePointer Function [[array]]
; CHECK: OpLabel
; CHECK-NEXT: OpVariable [[array_ptr]]
; CHECK-NOT: OpVariable
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "dont_touch_spec_constant_access_chain"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%uint_1 = OpConstant %uint 1
%array = OpTypeArray %uint %uint_1
%uint_ptr = OpTypePointer Function %uint
%array_ptr = OpTypePointer Function %array
%uint_0 = OpConstant %uint 0
%spec_const = OpSpecConstant %uint 0
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %array_ptr Function
%3 = OpAccessChain %uint_ptr %var %spec_const
OpStore %3 %uint_0 Volatile
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, NoPartialAccesses) {
  const std::string text = R"(
;
; CHECK: [[struct:%\w+]] = OpTypeStruct
; CHECK: [[struct_ptr:%\w+]] = OpTypePointer Function [[struct]]
; CHECK: OpLabel
; CHECK-NEXT: OpVariable [[struct_ptr]]
; CHECK-NOT: OpVariable
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "no_partial_accesses"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%const = OpConstantNull %struct1
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
OpStore %var %const
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, DontTouchPtrAccessChain) {
  const std::string text = R"(
;
; CHECK: [[struct:%\w+]] = OpTypeStruct
; CHECK: [[struct_ptr:%\w+]] = OpTypePointer Function [[struct]]
; CHECK: OpLabel
; CHECK-NEXT: OpVariable [[struct_ptr]]
; CHECK-NOT: OpVariable
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "dont_touch_ptr_access_chain"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%uint_0 = OpConstant %uint 0
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
%3 = OpPtrAccessChain %uint_ptr %var %uint_0 %uint_0
OpStore %3 %uint_0
%4 = OpAccessChain %uint_ptr %var %uint_0
OpStore %4 %uint_0
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, false);
}

TEST_F(ScalarReplacementTest, DontTouchInBoundsPtrAccessChain) {
  const std::string text = R"(
;
; CHECK: [[struct:%\w+]] = OpTypeStruct
; CHECK: [[struct_ptr:%\w+]] = OpTypePointer Function [[struct]]
; CHECK: OpLabel
; CHECK-NEXT: OpVariable [[struct_ptr]]
; CHECK-NOT: OpVariable
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "dont_touch_in_bounds_ptr_access_chain"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%uint_0 = OpConstant %uint 0
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
%3 = OpInBoundsPtrAccessChain %uint_ptr %var %uint_0 %uint_0
OpStore %3 %uint_0
%4 = OpInBoundsAccessChain %uint_ptr %var %uint_0
OpStore %4 %uint_0
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, false);
}

TEST_F(ScalarReplacementTest, DonTouchAliasedDecoration) {
  const std::string text = R"(
;
; CHECK: [[struct:%\w+]] = OpTypeStruct
; CHECK: [[struct_ptr:%\w+]] = OpTypePointer Function [[struct]]
; CHECK: OpLabel
; CHECK-NEXT: OpVariable [[struct_ptr]]
; CHECK-NOT: OpVariable
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "aliased"
OpDecorate %var Aliased
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%uint_0 = OpConstant %uint 0
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
%3 = OpAccessChain %uint_ptr %var %uint_0
%4 = OpLoad %uint %3
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, CopyRestrictDecoration) {
  const std::string text = R"(
;
; CHECK: OpName
; CHECK-NEXT: OpDecorate [[var0:%\w+]] Restrict
; CHECK-NEXT: OpDecorate [[var1:%\w+]] Restrict
; CHECK: [[int:%\w+]] = OpTypeInt
; CHECK: [[struct:%\w+]] = OpTypeStruct
; CHECK: [[int_ptr:%\w+]] = OpTypePointer Function [[int]]
; CHECK: [[struct_ptr:%\w+]] = OpTypePointer Function [[struct]]
; CHECK: OpLabel
; CHECK-NEXT: [[var1]] = OpVariable [[int_ptr]]
; CHECK-NEXT: [[var0]] = OpVariable [[int_ptr]]
; CHECK-NOT: OpVariable [[struct_ptr]]
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "restrict"
OpDecorate %var Restrict
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%struct1 = OpTypeStruct %uint %uint
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%uint_0 = OpConstant %uint 0
%uint_1 = OpConstant %uint 1
%func = OpTypeFunction %void
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
%3 = OpAccessChain %uint_ptr %var %uint_0
%4 = OpLoad %uint %3
%5 = OpAccessChain %uint_ptr %var %uint_1
%6 = OpLoad %uint %5
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, DontClobberDecoratesOnSubtypes) {
  const std::string text = R"(
;
; CHECK: OpDecorate [[array:%\w+]] ArrayStride 1
; CHECK: [[uint:%\w+]] = OpTypeInt 32 0
; CHECK: [[array]] = OpTypeArray [[uint]]
; CHECK: [[array_ptr:%\w+]] = OpTypePointer Function [[array]]
; CHECK: OpLabel
; CHECK-NEXT: OpVariable [[array_ptr]] Function
; CHECK-NOT: OpVariable
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "array_stride"
OpDecorate %array ArrayStride 1
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%uint_1 = OpConstant %uint 1
%array = OpTypeArray %uint %uint_1
%struct1 = OpTypeStruct %array
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%uint_0 = OpConstant %uint 0
%func = OpTypeFunction %void %uint
%1 = OpFunction %void None %func
%param = OpFunctionParameter %uint
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
%3 = OpAccessChain %uint_ptr %var %uint_0 %param
%4 = OpLoad %uint %3
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}

TEST_F(ScalarReplacementTest, DontCopyMemberDecorate) {
  const std::string text = R"(
;
; CHECK-NOT: OpDecorate
; CHECK: [[uint:%\w+]] = OpTypeInt 32 0
; CHECK: [[struct:%\w+]] = OpTypeStruct [[uint]]
; CHECK: [[uint_ptr:%\w+]] = OpTypePointer Function [[uint]]
; CHECK: [[struct_ptr:%\w+]] = OpTypePointer Function [[struct]]
; CHECK: OpLabel
; CHECK-NEXT: OpVariable [[uint_ptr]] Function
; CHECK-NOT: OpVariable
;
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
OpName %func "member_decorate"
OpMemberDecorate %struct1 0 Offset 1
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%uint_1 = OpConstant %uint 1
%struct1 = OpTypeStruct %uint
%uint_ptr = OpTypePointer Function %uint
%struct1_ptr = OpTypePointer Function %struct1
%uint_0 = OpConstant %uint 0
%func = OpTypeFunction %void %uint
%1 = OpFunction %void None %func
%2 = OpLabel
%var = OpVariable %struct1_ptr Function
%3 = OpAccessChain %uint_ptr %var %uint_0
%4 = OpLoad %uint %3
OpReturn
OpFunctionEnd
  )";

  SinglePassRunAndMatch<opt::ScalarReplacementPass>(text, true);
}
#endif  // SPIRV_EFFCEE

}  // namespace
