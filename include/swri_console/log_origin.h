// *****************************************************************************
//
// Copyright (c) 2015, Southwest Research Institute® (SwRI®)
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Southwest Research Institute® (SwRI®) nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Southwest Research Institute® BE LIABLE 
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// *****************************************************************************
#ifndef SWRI_CONSOLE_LOG_ORIGIN_H_
#define SWRI_CONSOLE_LOG_ORIGIN_H_

namespace swri_console
{
struct LogOrigin
{
  std::string file;
  std::string function;
  uint32_t line;
  int node_id;
  uint8_t severity;

  bool operator<(const LogOrigin &other) const {
    // We'd like to return as quickly as possible, so the comparisons
    // are ordered by integer components before string components.
    // Within each class, they are ordred from most unique to least
    // unique.    
    if (line != other.line) {
      return line < other.line;
    } else {
      if (node_id != other.node_id) {
        return node_id < other.node_id;
      } else {
        if (severity != other.severity) {
          return severity < other.severity;
        } else {
          if (function != other.function) {
            return function < other.function;
          } else {
            return file < other.file;
          }
        }
      }      
    }
  }
};  // struct LogOrigin
}  // namespace swri_console
#endif  // SWRI_CONSOLE_LOG_ORIGIN_H_

