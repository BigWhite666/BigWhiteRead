Mem = {
	Pid = 0
}

print("欢迎使用懒人内存插件 当前版本号1.0,懒人精灵交流QQ群:308254130")
work = getWorkPath()--脚本工作目录
local type = getCpuArch()--获取cpu架构
extractAssets("内存插件.rc" , work)--释放内存so库
local sopath = ""
print("当前CPU结构"..type)
if type == 0 then
	sopath = work .. "/liblrapi_x86.so"
	print("当前系统架构：x86")
elseif type == 1 then
	sopath = work .. "/liblrapi_arm.so"
	print("当前系统架构：arm")
elseif type == 2 then
	sopath = work .. "/liblrapi_arm64.so"
	print("当前系统架构：arm64")
elseif type == 3 then
	sopath = work .. "/liblrapi_x86_64.so"
	print("当前系统架构：x86_64")
end

-- 定义 C 函数的原型
ffi.cdef[[
typedef struct {
long* addrs;
int count;
} AddressData;

typedef struct {
long long value;
int type;
unsigned long offset;
} SearchCondition;



typedef struct  {
    long* addresses;
    int count;
} SearchResult;

SearchResult BigWhite_SearchWithOffset(int BigWhitePid, const SearchCondition* conditions, int conditionCount, int mem);
void BigWhite_FreeSearchResult(SearchResult* result);

int BigWhite_GetPID(const char *packageName);
int BigWhite_GetPID2(const char *packageName);
unsigned long BigWhite_GetModuleBase(int pid, const char *module_name);
unsigned long BigWhite_GetPtr64(int BigWhitePid, unsigned long addr);
unsigned long BigWhite_GetPtr32(int BigWhitePid, unsigned long addr);
int BigWhite_GetDword(int BigWhitePid, unsigned long addr);
float BigWhite_GetFloat(int BigWhitePid, unsigned long addr);
bool BigWhite_WriteDword(int BigWhitePid, unsigned long addr, int data);
bool BigWhite_WriteFloat(int BigWhitePid, unsigned long addr, float data);

AddressData Search_DWORD(int BigWhitePid, int value, int mem);

AddressData Search_FLOAT(int BigWhitePid, float value, int mem);

AddressData Search_BYTE(int BigWhitePid, char value, int mem);

AddressData Search_WORD(int BigWhitePid, short value, int mem);

AddressData Search_QWORD(int BigWhitePid, long long value, int mem);

AddressData Search_XOR(int BigWhitePid, int value, int mem);

AddressData Search_DOUBLE(int BigWhitePid, double value, int mem);

]]
--[===[	Mem_Auto 0
Mem_A    1
Mem_Ca   2
Mem_Cd   3
Mem_Cb   4
Mem_Jh   5
Mem_J    6
Mem_S    7
Mem_V    8
Mem_Xa   9
Mem_Xs   10
Mem_As   11
Mem_B    12
Mem_O    13]===]

-- 加载共享库
local mylib = ffi.load(sopath)

-- 检查是否成功加载
if mylib then
	print("内存插件加载成功！")
else
	print("内存插件加载失败！")
	exitScript()
end

function Mem.GetPID(PackageName)
	Mem.Pid = mylib.BigWhite_GetPID(PackageName)
	return Mem.Pid
end
function Mem.GetPID2(PackageName)
	Mem.Pid = mylib.BigWhite_GetPID2(PackageName)
	return Mem.Pid
end

function Mem.GetModuleBase(Lib)
	return mylib.BigWhite_GetModuleBase(Mem.Pid , Lib)
end

function Mem.GetPtr64(Addr)
	return mylib.BigWhite_GetPtr64(Mem.Pid , Addr)
end
function Mem.GetPtr32(Addr)
	return mylib.BigWhite_GetPtr32(Mem.Pid , Addr)
end
function Mem.GetDword(Addr)
	return mylib.BigWhite_GetDword(Mem.Pid , Addr)
end
function Mem.GetFloat(Addr)
	return mylib.BigWhite_GetFloat(Mem.Pid , Addr)
end
function Mem.WriteDword(Addr , value)
	return mylib.BigWhite_WriteDword(Mem.Pid , Addr , value)
end
function Mem.WriteFloat(Addr , value)
	return mylib.BigWhite_WriteFloat(Mem.Pid , Addr , value)
end

-- 修改搜索函数实现
function Mem.Search_DWORD(value, mem)
    return mylib.Search_DWORD(Mem.Pid, value, mem)
end

function Mem.Search_FLOAT(value, mem)
    return mylib.Search_FLOAT(Mem.Pid, value, mem)
end

function Mem.Search_BYTE(value, mem)
    return mylib.Search_BYTE(Mem.Pid, value, mem)
end

function Mem.Search_WORD(value, mem)
    return mylib.Search_WORD(Mem.Pid, value, mem)
end

function Mem.Search_QWORD(value, mem)
    return mylib.Search_QWORD(Mem.Pid, value, mem)
end

function Mem.Search_XOR(value, mem)
    return mylib.Search_XOR(Mem.Pid, value, mem)
end

function Mem.Search_DOUBLE(value, mem)
    return mylib.Search_DOUBLE(Mem.Pid, value, mem)
end
function Mem.SearchWithOffset(conditions, mem)
    if not conditions or #conditions == 0 then
        return {count = 0, addresses = {}}
    end
    
    local c_conditions = ffi.new("SearchCondition[?]", #conditions)
    
    for i, condition in ipairs(conditions) do
        if not condition.value or not condition.type or not condition.offset then
            return {count = 0, addresses = {}}
        end
        
        c_conditions[i-1].value = tonumber(condition.value) or 0
        c_conditions[i-1].type = tonumber(condition.type) or 0
        c_conditions[i-1].offset = tonumber(condition.offset) or 0
    end
    
    local result = mylib.BigWhite_SearchWithOffset(Mem.Pid, c_conditions, #conditions, mem)
    
    if not result or result.count <= 0 or not result.addresses then
        return {count = 0, addresses = {}}
    end
    
    local results = {
        count = 0,
        addresses = {}
    }
    
    for i = 0, result.count - 1 do
        local addr = result.addresses[i]
        if addr and addr ~= 0 then
            results.count = results.count + 1
            table.insert(results.addresses, addr)
        end
    end
    
    Mem.FreeSearchResult(ffi.new("SearchResult*", result))
    
    return results
end


function Mem.FreeSearchResult(result)
    mylib.BigWhite_FreeSearchResult(result)
end

function ReadPointer(...)
	local ResultAddr = 0
	for i , v in ipairs({...}) do
		ResultAddr = Mem.GetPtr64(ResultAddr + v)
	end
	return ResultAddr
end

