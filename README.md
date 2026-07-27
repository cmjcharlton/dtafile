# DTAFile - C++ class to read/write/modify Stata .dta format files

Note that when variables/observations are referenced by index these are one-based to correspond with the Stata convention. For examples of usage see the src directory.

For the official description of the file format see https://www.stata.com/help.cgi?dta


#### Create/Load DTA file:

**DTAFile();**

Create empty DTAFile object



**DTAFile(std::uint8\_t version);**
Create empty DTAFile object in **version** format



**DTAFile(std::string\_view filename);**

Create DTAFile object from contents of filename



#### Save DTA file

**bool Save(std::string\_view filename);**

Store contents of DTAFile object into filename



**bool Save(std::string\_view filename, std::uint8\_t version = default\_version, std::endian byteorder = std::endian::native);**

Store contents of DTAFile object into filename, optionally setting version and byte order



#### DTA file information

**std::uint8\_t GetOriginalFileVersion();**
Returns the format version set when the DTAFile object was created



**std::endian GetOriginalByteOrder();**

Returns the byte order set when the DTAFile was created



**std::string GetDataLabel();**

Returns the data label for the DTAFile



**std::string GetTimeStamp();**

Returns the current time stamp for the DTAFile object



**std::uint64\_t GetNumObservations();**

Returns the number of observations (rows) in the DTAFile



**std::uint32\_t GetNumVariables();**

Returns the number of variables (columns) in the DTAFile



**std::vector<**[**std::string**](std::string)**> GetSortOrder();**

Returns a vector of variable names indicating the order in which they are sorted



**std::vector<**[**std::string**](std::string)**> GetCharacteristicNames(std::string\_view varname);**

Returns a vector of characteristic names (attributes) associated with the DTAFile



**std::string GetCharacteristic(std::string\_view varname, std::string\_view charname);**

Returns the contents of the named characteristic if it is defined



#### Variable information

**std::vector<**[**std::string**](std::string)**> GetVariableNames();**

Returns a list containing the names for all the variables in the DTAFile



**std::string GetVariableName(std::uint32\_t varnumber);**

Returns the name for the variable in position *varnumber*



**std::string GetVariableLabel(std::string\_view varname);**

Returns the label associated with the variable



**std::string GetValueLabelName(std::string\_view varname);**

Returns the name of the value label table associated with this variable



**std::string GetVariableType(std::string\_view varname);**

Returns a string indicating the type of the chosen variable



**std::string GetFormat(std::string\_view varname);**

Returns the format string associated with the variable



**std::size\_t GetMaxStringLength(std::string\_view varname);**

If the variable is of string type returns the maximum length that it can hold, otherwise zero



#### Value label information

**std::vector<**[**std::string**](std::string)**> GetValueLabelNames();**

Returns a list of defined value label names



**std::vector<**[**std::int32\_t**](std::int32\\\\_t)**> GetValueLabelCodes(std::string labname);**

Returns a list of codes associated with a value label



**std::string GetValueLabelString(std::string labname, std::int32\_t value);**

Returns the label associated with the requested value label and code



#### Data values

**ColumnData GetVariable(std::string\_view varname);**

Returns a copy of the data associated with the chosen variable name



**std::vector<CellData> GetObservation(std::uint64\_t row);**

Returns a vector of values corresponding to the chosen observation



**CellData GetValue(std::string\_view varname, std::uint64\_t row);**

Returns the cell value for the chosen observation of a variable



**T GetValue(std::string\_view varname, std::uint64\_t row);**

Returns the cell value in the specified type of the chosen observation of a variable



**std::string GetValueString(std::string\_view varname, std::uint64\_t row);**

Returns a string represenation for a cell value for the chosen observation of a variable (missing data is signified by .)



#### Modify current DTA file

**bool AddVariable(std::string\_view varname, std::string\_view type);**

Add a new variable with the chosen name and type to DTAFile



**bool AddValueLabel(std::string labname, std::int32\_t value, std::string label);**

Add a new entry to the value label table corresponding to labname



**bool AddCharacteristic(std::string\_view varname, std::string\_view charname, std::string characteristic);**

Add a new characteristic corresponding to varname to DTAFile. If varname is "\_dta" then is global



**void StampTime();**

Stamp DTAFile with the current time



**bool SetDataLabel(std::string\_view label);**

Set the descriptive label for DTAFile



**bool SetVariableLabel(std::string\_view varname, std::string\_view label);**

Set the descriptive label for the chosen variable



**bool SetFormat(std::string\_view varname, std::string\_view format);**

Set the display format string for the chosen variable



**bool SetObservations(std::uint64\_t obs);**

Set the number of observations in DTAFile



**bool SetSortOrder(const std::vector<**[**std::string\_view**](std::string\\\\_view)**> \&variablelist);**

Set the stored sort order of variable in DTAFile



**bool AssignValueLabel(std::string\_view varname, std::string\_view labname);**

Associate a value label definition with a variable



**bool SetValue(std::string\_view varname, std::uint64\_t row, T value);**

Set the a cell value corresponding to the variable and row



**void SortData(const std::vector**[**std::string\\\_view**](std::string\\\\_view) **keys);**

Sort the data in DTAFile by the specified keys



**bool RenameVariable(std::string\_view oldname, std::string\_view newname);**

Change the name of a variable



**bool RemoveVariable(std::string\_view varname);**

Remove an entire variable from the DTAFile



**bool RenameValueLabel(std::string oldname, std::string newname);**

Change the name of a value label



**bool RemoveValueLabel(std::string labname, std::int32\_t value);**

Remove a definition of a value label



**bool RemoveCharacteristic(std::string\_view varname, std::string\_view charname);**

Remove a previously defined characteristic

