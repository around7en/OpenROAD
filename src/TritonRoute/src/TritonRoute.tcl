#############################################################################
#
# BSD 3-Clause License
#
# Copyright (c) 2020, The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#############################################################################

sta::define_cmd_args "detailed_route" {
    -param filename
}

proc detailed_route { args } {
  sta::parse_key_args "detailed_route" args keys {-param}
  sta::check_argc_eq0 "detailed_route" $args

  if { ![info exists keys(-param)] } {
    sta::cmd_usage_error "detailed_route"
  }
  tr::detailed_route_cmd $keys(-param)
}

sta::define_cmd_args "detailed_route_debug" {
    [-dr]
    [-net name]
}

proc detailed_route_debug { args } {
  sta::parse_key_args "detailed_route_debug" args \
      keys {-net} \
      flags {-dr}

  sta::check_argc_eq0 "detailed_route_debug" $args

  set dr [info exists flags(-dr)]
  if { [info exists keys(-net)] } {
    set net_name $keys(-net)
  } else {
    set net_name ""
  }

  tr::set_detailed_route_debug_cmd $net_name $dr
}
