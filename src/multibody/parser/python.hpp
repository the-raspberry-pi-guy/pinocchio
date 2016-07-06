//
// Copyright (c) 2016 CNRS
//
// This file is part of Pinocchio
// Pinocchio is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// Pinocchio is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Lesser Public License for more details. You should have
// received a copy of the GNU Lesser General Public License along with
// Pinocchio If not, see
// <http://www.gnu.org/licenses/>.

#ifndef __se3_pythonparser_hpp__
#define __se3_pythonparser_hpp__

#include "pinocchio/multibody/model.hpp"

namespace se3
{
  namespace python
  {
    Model buildModel (const std::string & filename, bool verbose = false);
  } // namespace python
} // namespace se3

#endif // ifndef __se3_pythonparser_hpp__
