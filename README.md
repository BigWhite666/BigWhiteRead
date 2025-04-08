# BigWhiteRead

针对 Linux 平台上远程进程内存读写和搜索的工具实现

## 简介

BigWhiteRead是一个用于Linux平台的内存操作工具，支持远程进程内存读写和搜索功能。当前版本为1.0，支持多种CPU架构，包括x86、arm、arm64和x86_64。工具利用Linux系统调用`process_vm_readv`和`process_vm_writev`实现对远程进程内存的读写操作。

## 功能特点

- 支持多种CPU架构（x86、arm、arm64、x86_64）
- 进程内存读写（DWORD、FLOAT、指针等）
- 多种数据类型搜索（DWORD、FLOAT、BYTE、WORD、QWORD、XOR、DOUBLE）
- 支持偏移量搜索
- 支持多级指针操作
- 支持特征码搜索

## 环境要求

- Linux操作系统
- 支持的CPU架构：x86、arm、arm64、x86_64
- 需要具有对目标进程的访问权限（通常需要root权限）

## 懒人精灵安装方法

工具会自动根据系统CPU架构释放对应的SO库文件：
- x86：liblrapi_x86.so
- arm：liblrapi_arm.so
- arm64：liblrapi_arm64.so
- x86_64：liblrapi_x86_64.so

初始化代码会自动检测CPU架构并加载相应的库文件：

```lua
work = getWorkPath()  -- 脚本工作目录
local type = getCpuArch()  -- 获取CPU架构
extractAssets("内存插件.rc", work)  -- 释放内存SO库
```

## API详细参考

### 初始化

```lua
-- 初始化内存操作对象
Mem = {
    Pid = 0  -- 存储目标进程的PID
}
```

### 进程操作

#### 获取进程PID

```lua
-- 方法1: 获取指定包名的进程PID
Mem.GetPID(packageName)

-- 参数:
-- packageName: string - 目标进程的包名
-- 返回值: number - 进程PID，失败返回0

-- 示例:
local pid = Mem.GetPID("com.example.app")
print("进程PID:", pid)  -- 输出可能为: 进程PID: 12345
```

```lua
-- 方法2: 获取指定包名的最后一个进程PID
Mem.GetPID2(packageName)

-- 参数:
-- packageName: string - 目标进程的包名
-- 返回值: number - 最后一个匹配的进程PID，未找到返回0

-- 示例:
local pid = Mem.GetPID2("com.example.app")
print("最后匹配的进程PID:", pid)  -- 输出可能为: 最后匹配的进程PID: 12345
```

两个函数的区别：
- `GetPID`: 返回第一个匹配的进程PID
- `GetPID2`: 返回最后一个匹配的进程PID，适用于有多个同名进程的情况

#### 获取模块基址

```lua
-- 获取指定进程中模块的基址
Mem.GetModuleBase(libName)

-- 参数:
-- libName: string - 模块名称，例如"libil2cpp.so"
-- 返回值: number - 模块基址，未找到返回0

-- 示例:
local base = Mem.GetModuleBase("libil2cpp.so")
print("模块基址:", string.format("0x%X", base))  -- 输出可能为: 模块基址: 0x7F1234000000
```

### 内存读写

#### 读取内存

```lua
-- 读取64位指针
Mem.GetPtr64(addr)

-- 参数:
-- addr: number - 内存地址
-- 返回值: number - 64位指针值

-- 示例:
local ptr = Mem.GetPtr64(0x7F1234000000)
print("64位指针值:", string.format("0x%X", ptr))  -- 输出可能为: 64位指针值: 0x7F1234001000
```

```lua
-- 读取32位指针
Mem.GetPtr32(addr)

-- 参数:
-- addr: number - 内存地址
-- 返回值: number - 32位指针值

-- 示例:
local ptr = Mem.GetPtr32(0x7F1234000000)
print("32位指针值:", string.format("0x%X", ptr))  -- 输出可能为: 32位指针值: 0x12340000
```

```lua
-- 读取DWORD值(32位整数)
Mem.GetDword(addr)

-- 参数:
-- addr: number - 内存地址
-- 返回值: number - 32位整数值

-- 示例:
local value = Mem.GetDword(0x7F1234000000)
print("DWORD值:", value)  -- 输出可能为: DWORD值: 1234567
```

```lua
-- 读取浮点数
Mem.GetFloat(addr)

-- 参数:
-- addr: number - 内存地址
-- 返回值: number - 浮点数值

-- 示例:
local value = Mem.GetFloat(0x7F1234000000)
print("浮点数值:", value)  -- 输出可能为: 浮点数值: 123.456
```

#### 写入内存

```lua
-- 写入DWORD值(32位整数)
Mem.WriteDword(addr, value)

-- 参数:
-- addr: number - 内存地址
-- value: number - 要写入的32位整数值
-- 返回值: boolean - 写入是否成功

-- 示例:
local success = Mem.WriteDword(0x7F1234000000, 9999)
print("写入是否成功:", success)  -- 输出可能为: 写入是否成功: true
```

```lua
-- 写入浮点数
Mem.WriteFloat(addr, value)

-- 参数:
-- addr: number - 内存地址
-- value: number - 要写入的浮点数值
-- 返回值: boolean - 写入是否成功

-- 示例:
local success = Mem.WriteFloat(0x7F1234000000, 999.99)
print("写入是否成功:", success)  -- 输出可能为: 写入是否成功: true
```

### 内存搜索

#### 基本类型搜索

所有搜索函数都遵循相同的参数结构：

```lua
-- 通用搜索函数形式
Mem.Search_XXX(value, memArea)

-- 参数:
-- value: 要搜索的值（类型取决于具体函数）
-- memArea: number - 内存区域类型（0表示搜索所有区域）
-- 返回值: table - 包含找到的地址列表和数量
--   结构: { addrs = {长整型地址数组}, count = 结果数量 }
```

具体搜索函数：

```lua
-- 搜索DWORD值(32位整数)
Mem.Search_DWORD(value, memArea)

-- 示例:
local result = Mem.Search_DWORD(12345, 0)
print("找到结果数量:", result.count)
-- 输出第一个找到的地址（如果有）
if result.count > 0 then
    print("第一个地址:", string.format("0x%X", result.addrs[1]))
end
```

```lua
-- 搜索浮点数
Mem.Search_FLOAT(value, memArea)

-- 示例:
local result = Mem.Search_FLOAT(123.45, 0)
print("找到结果数量:", result.count)
```

```lua
-- 搜索字节
Mem.Search_BYTE(value, memArea)

-- 示例:
local result = Mem.Search_BYTE(255, 0)  -- 搜索值为255的字节
print("找到结果数量:", result.count)
```

```lua
-- 搜索WORD(16位整数)
Mem.Search_WORD(value, memArea)

-- 示例:
local result = Mem.Search_WORD(12345, 0)
print("找到结果数量:", result.count)
```

```lua
-- 搜索QWORD(64位整数)
Mem.Search_QWORD(value, memArea)

-- 示例:
local result = Mem.Search_QWORD(1234567890123, 0)
print("找到结果数量:", result.count)
```

```lua
-- 搜索XOR加密值
Mem.Search_XOR(value, memArea)

-- 示例:
local result = Mem.Search_XOR(12345, 0)
print("找到结果数量:", result.count)
```

```lua
-- 搜索双精度浮点数
Mem.Search_DOUBLE(value, memArea)

-- 示例:
local result = Mem.Search_DOUBLE(123.45678, 0)
print("找到结果数量:", result.count)
```

#### 带偏移量的搜索

```lua
-- 带偏移量的多条件搜索
Mem.SearchWithOffset(conditions, memArea)

-- 参数:
-- conditions: table - 搜索条件数组，每个条件包含:
--   value: number - 搜索值
--   type: number - 数据类型（0=DWORD, 1=FLOAT等）
--   offset: number - 相对于基址的偏移量
-- memArea: number - 内存区域类型（0表示搜索所有区域）
-- 返回值: table - 包含找到的地址列表和数量
--   结构: { addresses = {长整型地址数组}, count = 结果数量 }

-- 示例:
local conditions = {
    {value = 100, type = 0, offset = 0},    -- 基地址处的DWORD值为100
    {value = 3.14, type = 1, offset = 4},   -- 基地址+4处的FLOAT值为3.14
    {value = 200, type = 0, offset = 8}     -- 基地址+8处的DWORD值为200
}
local results = Mem.SearchWithOffset(conditions, 0)
print("符合条件的地址数量:", results.count)

-- 遍历所有结果
for i = 1, results.count do
    print("地址" .. i .. ":", string.format("0x%X", results.addresses[i]))
end
```

### 内存区域类型

内存区域类型常量，用于指定搜索范围：

```lua
-- 常量定义
Mem_Auto = 0  -- 所有内存页
Mem_A = 1     -- 通常为进程主要内存区域
Mem_Ca = 2    -- 堆内存区域(libc_malloc)
Mem_Cd = 3    -- 数据区域(/data/data/)
Mem_Cb = 4    -- BSS段(:bss)
Mem_Jh = 5    -- 堆内存
Mem_J = 6     -- Java堆内存([anon:dalvik)
Mem_S = 7     -- 栈内存([stack])
Mem_V = 8     -- 显存(/dev/kgsl-3d0)
Mem_Xa = 9    -- 应用程序代码区域(/data/app/)
Mem_Xs = 10   -- 系统框架区域(/system/framework/)
Mem_As = 11   -- 共享内存区域(/dev/ashmem/)
Mem_B = 12    -- 系统字体区域(/system/fonts/)
Mem_O = 13    -- 其他内存区域
```

### 辅助函数

```lua
-- 读取多级指针
ReadPointer(...)

-- 参数:
-- ...: 可变参数，表示多级指针的偏移链
-- 返回值: number - 最终指针指向的地址

-- 示例:
-- 假设内存结构: 基址 -> 偏移0x10 -> 偏移0x20 -> 目标值
local baseAddr = 0x7F1234000000
local finalAddr = ReadPointer(baseAddr, 0x10, 0x20)
print("最终地址:", string.format("0x%X", finalAddr))
-- 等价于以下手动操作:
local addr1 = Mem.GetPtr64(baseAddr + 0x10)
local finalAddr = Mem.GetPtr64(addr1 +, 0x20)
```

```lua
-- 释放搜索结果
Mem.FreeSearchResult(result)

-- 参数:
-- result: table - 通过SearchWithOffset函数获得的搜索结果
-- 返回值: 无

-- 示例:
local results = Mem.SearchWithOffset(conditions, 0)
-- 使用结果...
Mem.FreeSearchResult(results)  -- 释放资源
```

## 完整使用示例

### 示例1：简单内存修改

```lua
-- 初始化并连接到指定进程
Mem.GetPID("com.example.game")
if Mem.Pid == 0 then
    print("未找到目标进程")
    return
end

-- 获取游戏主模块基址
local baseAddr = Mem.GetModuleBase("libil2cpp.so")
if baseAddr == 0 then
    print("未找到游戏模块")
    return
end

-- 读取角色金币数量(假设在基址+0x12345678的位置)
local goldAddr = baseAddr + 0x12345678
local goldCount = Mem.GetDword(goldAddr)
print("当前金币:", goldCount)

-- 修改金币数量
Mem.WriteDword(goldAddr, 99999)
print("修改后金币:", Mem.GetDword(goldAddr))
```

### 示例2：搜索并修改内存值

```lua
-- 初始化并连接到指定进程
Mem.GetPID("com.example.game")
if Mem.Pid == 0 then
    print("未找到目标进程")
    return
end

-- 假设玩家当前生命值为100，搜索这个值
print("请确保角色生命值为100，然后开始搜索...")
local results = Mem.Search_DWORD(100, 0)
print("初次搜索结果:", results.count)

-- 假设玩家受到伤害，生命值变为80
print("请让角色受到伤害，生命值变为80，然后进行第二次搜索...")

-- 筛选出从100变为80的地址
local validAddrs = {}
for i = 1, results.count do
    local addr = results.addrs[i]
    local value = Mem.GetDword(addr)
    if value == 80 then
        table.insert(validAddrs, addr)
    end
end
print("筛选后可能的生命值地址数量:", #validAddrs)

-- 修改所有可能的地址
for i, addr in ipairs(validAddrs) do
    Mem.WriteDword(addr, 999)
    print("修改地址:", string.format("0x%X", addr))
end
print("已将所有可能的生命值修改为999")
```

### 示例3：多级指针搜索

```lua
-- 初始化并连接到指定进程
Mem.GetPID("com.example.game")
if Mem.Pid == 0 then
    print("未找到目标进程")
    return
end

-- 获取游戏主模块基址
local baseAddr = Mem.GetModuleBase("libil2cpp.so")
if baseAddr == 0 then
    print("未找到游戏模块")
    return
end

-- 使用多级指针找到目标地址
-- 假设指针链为: 基址 + 0x1234000 -> +0x20 -> +0x10 -> +0x8 -> 目标值
local targetAddr = ReadPointer(baseAddr + 0x1234000, 0x20, 0x10, 0x8)
print("目标地址:", string.format("0x%X", targetAddr))

-- 读取目标地址的值
local value = Mem.GetFloat(targetAddr)
print("当前值:", value)

-- 修改值
Mem.WriteFloat(targetAddr, 999.999)
print("修改后的值:", Mem.GetFloat(targetAddr))
```

### 示例4：带偏移的条件搜索

```lua
-- 初始化并连接到指定进程
Mem.GetPID("com.example.game")
if Mem.Pid == 0 then
    print("未找到目标进程")
    return
end

-- 假设游戏角色数据结构如下:
-- 偏移0: 生命值(DWORD) = 100
-- 偏移4: 魔法值(DWORD) = 50
-- 偏移8: 攻击力(FLOAT) = 25.5
local conditions = {
    {value = 100, type = 0, offset = 0},  -- 生命值
    {value = 50, type = 0, offset = 4},   -- 魔法值
    {value = 25.5, type = 1, offset = 8}  -- 攻击力
}

-- 搜索符合上述结构的内存地址
local results = Mem.SearchWithOffset(conditions, 0)
print("找到符合条件的地址数量:", results.count)

-- 修改所有符合条件的地址
for i = 1, results.count do
    local baseAddr = results.addresses[i]
    -- 修改生命值
    Mem.WriteDword(baseAddr, 999)
    -- 修改魔法值
    Mem.WriteDword(baseAddr + 4, 999)
    -- 修改攻击力
    Mem.WriteFloat(baseAddr + 8, 999.9)
    print("已修改地址:", string.format("0x%X", baseAddr))
end

-- 释放搜索结果
Mem.FreeSearchResult(results)
```

## 注意事项

- 使用前请确保已获取正确的目标进程PID
- 内存操作可能需要root权限
- 对内存进行修改可能导致目标应用程序崩溃，请谨慎操作
- 搜索结果使用后需要调用`Mem.FreeSearchResult()`释放资源
- 不同的内存区域有不同的权限，请根据实际情况选择合适的内存区域
- 进行内存修改操作前，建议先备份重要数据
- 工具的使用受限于Linux系统权限，某些系统上可能无法正常工作

## 交流与支持

懒人精灵交流QQ群: 308254130

---

© 2023 BigWhiteRead 项目团队
