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

SEP			= ^\
COMBINE		= &
CPD			= xcopy /y /e /h
RMLIBGEN	= if exist $(LIB_GEN_PATH) $(RD) $(LIB_GEN_PATH)
RMDEBUG		= if exist $(DEBUG_DIR) $(RD) $(DEBUG_DIR)
RMOUTPUT	= if exist $(OUTPUT_DIR) $(RD) $(OUTPUT_DIR)

BUILD_LIB_PREFIX	=

!INCLUDE $(MODULE_DIR)/CommonMakefileVars

set_vars:
	@set PACKAGES_PATH=%PACKAGES_PATH%;$(LIB_GEN_LOCATION)
	@set PRIVATE_DATA_OBJ=$(PRIVATE_DATA_OBJ)
	@set PRIVATE_DATA_LIB=$(PRIVATE_DATA_LIB)
	@set PRIVATE_DATA_OBJFILES=$(PRIVATE_DATA_OBJFILES)

!INCLUDE $(MODULE_DIR)\CommonMakefile
