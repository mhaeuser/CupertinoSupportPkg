## file
#
#  Copyright (C) 2017, CupertinoNet.  All rights reserved.<BR>
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#
##

SEP			= /
COMBINE		= ;
CPD			= $(CP) -r
RMLIBGEN	= $(RD) $(LIB_GEN_PATH)
RMDEBUG		= $(RD) $(DEBUG_DIR)
RMOUTPUT	= $(RD) $(OUTPUT_DIR)

BUILD_LIB_PREFIX	= export PACKAGES_PATH=$(PACKAGES_PATH):$(LIB_GEN_LOCATION);

include $(MODULE_DIR)/CommonMakefileVars

export PRIVATE_DATA_OBJ
export PRIVATE_DATA_LIB
export PRIVATE_DATA_OBJFILES

set_vars:
	

include $(MODULE_DIR)/CommonMakefile
