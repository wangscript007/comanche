/*
   Copyright [2017-2019] [IBM Corporation]
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


/*
 * Authors:
 *
 */

#include "fd_unblock_set_monitor.h"

#include <iostream> /* cerr */

using guard = std::lock_guard<std::mutex>;

fd_unblock_set_monitor:: fd_unblock_set_monitor(std::mutex &m_, std::set<int> &s_, int fd_)
  : _m(m_)
  , _s(s_)
  , _fd(fd_)
{
  guard g{_m};
  _s.insert(_fd);
}

fd_unblock_set_monitor::~fd_unblock_set_monitor()
try
{
  guard g{_m};
  _s.erase(_fd);
}
catch ( const std::exception &e )
{
  std::cerr << __func__ << " exception " << e.what() << "\n";
}
