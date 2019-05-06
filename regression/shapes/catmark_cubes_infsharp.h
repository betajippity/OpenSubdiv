//
//   Copyright 2019 DreamWorks Animation LLC.
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

static const std::string catmark_cubes_infsharp =
"#\n"
"#   Four shapes ordered left->right and top->bottom in the XZ plane\n"
"#\n"
"#   Shape 1:  top-left\n"
"#\n"
"# 8 vertices\n"
"v -0.25 -0.50  1.25\n"
"v -0.25  0.50  1.25\n"
"v -1.25 -0.50  1.25\n"
"v -1.25  0.50  1.25\n"
"v -1.25 -0.50  0.25\n"
"v -1.25  0.50  0.25\n"
"v -0.25 -0.50  0.25\n"
"v -0.25  0.50  0.25\n"
"\n"
"# 14 uvs\n"
"vt 0.375 0.000\n"
"vt 0.625 0.000\n"
"vt 0.375 0.250\n"
"vt 0.625 0.250\n"
"vt 0.375 0.500\n"
"vt 0.625 0.500\n"
"vt 0.375 0.750\n"
"vt 0.625 0.750\n"
"vt 0.375 1.000\n"
"vt 0.625 1.000\n"
"vt 0.875 0.000\n"
"vt 0.875 0.250\n"
"vt 0.125 0.000\n"
"vt 0.125 0.250\n"
"\n"
"f  1/1   2/2   4/4   3/3\n"
"f  3/3   4/4   6/6   5/5\n"
"f  5/5   6/6   8/8   7/7\n"
"f  7/7   8/8   2/10  1/9\n"
"f  2/2   8/11  6/12  4/4\n"
"f  7/13  1/1   3/3   5/14\n"
"\n"
"t corner 1/1/0  0  10.0\n"
"\n"
"#\n"
"#   Shape 2:  top-right\n"
"#\n"
"v  1.25 -0.50  1.25\n"
"v  1.25  0.50  1.25\n"
"v  0.25 -0.50  1.25\n"
"v  0.25  0.50  1.25\n"
"v  0.25 -0.50  0.25\n"
"v  0.25  0.50  0.25\n"
"v  1.25 -0.50  0.25\n"
"v  1.25  0.50  0.25\n"
"\n"
"f  9/1  10/2  12/4  11/3\n"
"f 11/3  12/4  14/6  13/5\n"
"f 13/5  14/6  16/8  15/7\n"
"f 15/7  16/8  10/10  9/9\n"
"f 10/2  16/11 14/12 12/4\n"
"f 15/13  9/1  11/3  13/14\n"
"\n"
"t crease 2/1/0  8  9  10.0\n"
"t crease 2/1/0  8 10  10.0\n"
"\n"
"#\n"
"#   Shape 3:  bottom-left\n"
"#\n"
"v -0.25 -0.50 -0.25\n"
"v -0.25  0.50 -0.25\n"
"v -1.25 -0.50 -0.25\n"
"v -1.25  0.50 -0.25\n"
"v -1.25 -0.50 -1.25\n"
"v -1.25  0.50 -1.25\n"
"v -0.25 -0.50 -1.25\n"
"v -0.25  0.50 -1.25\n"
"\n"
"f 17/1  18/2  20/4  19/3\n"
"f 19/3  20/4  22/6  21/5\n"
"f 21/5  22/6  24/8  23/7\n"
"f 23/7  24/8  18/10 17/9\n"
"f 18/2  24/11 22/12 20/4\n"
"f 23/13 17/1  19/3  21/14\n"
"\n"
"t crease 2/1/0  18 16  10.0\n"
"t crease 2/1/0  16 22  10.0\n"
"t crease 2/1/0  22 20  10.0\n"
"t crease 2/1/0  18 20  10.0\n"
"\n"
"#\n"
"#   Shape 4:  bottom-right\n"
"#\n"
"v  1.25 -0.50 -0.25\n"
"v  1.25  0.50 -0.25\n"
"v  0.25 -0.50 -0.25\n"
"v  0.25  0.50 -0.25\n"
"v  0.25 -0.50 -1.25\n"
"v  0.25  0.50 -1.25\n"
"v  1.25 -0.50 -1.25\n"
"v  1.25  0.50 -1.25\n"
"\n"
"f 25/1  26/2  28/4  27/3\n"
"f 27/3  28/4  30/6  29/5\n"
"f 29/5  30/6  32/8  31/7\n"
"f 31/7  32/8  26/10 25/9\n"
"f 26/2  32/11 30/12 28/4\n"
"f 31/13 25/1  27/3  29/14\n"
"\n"
"t crease 2/1/0  24 25   0.1\n"
"t crease 2/1/0  25 27  10.0\n"
"t crease 2/1/0  27 26  10.0\n"
"t crease 2/1/0  26 24  10.0\n"
"t crease 2/1/0  28 29  10.0\n"
"t crease 2/1/0  29 31  10.0\n"
"t crease 2/1/0  31 30  10.0\n"
"t crease 2/1/0  30 28  10.0\n"
"t crease 2/1/0  24 30  10.0\n"
"t crease 2/1/0  25 31  10.0\n"
"t crease 2/1/0  26 28  10.0\n"
"t crease 2/1/0  27 29  10.0\n"
"\n"
"\n"
"t interpolateboundary 1/0/0 2\n"
"t facevaryinginterpolateboundary 1/0/0 1\n"
"\n"
;