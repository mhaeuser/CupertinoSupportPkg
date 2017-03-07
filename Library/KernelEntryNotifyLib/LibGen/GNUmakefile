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
RMDEBUG		= $(RD) $(DEBUG_DIR)
RMOUTPUT	= $(RD) $(OUTPUT_DIR)

include $(MODULE_DIR)/CommonMakefileVars

gen_c_write_header: gen_c_write_header_comments
	@echo \#include \<Base.h\>	>> $(DATA_FILE_PATH)

include $(MODULE_DIR)/CommonMakefile
