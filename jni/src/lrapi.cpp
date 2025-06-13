#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <malloc.h>
#include <math.h>
#include <thread>
#include <iostream>
#include <sys/stat.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <locale>
#include <string>
#include <codecvt>
#include <dlfcn.h>
#include <fstream>

#define EXPORT __attribute__((visibility("default")))

typedef unsigned short UTF16;
typedef char UTF8;

#if defined(__arm__)
int BigWhite_process_vm_readv_syscall = 376;
int BigWhite_process_vm_writev_syscall = 377;
#elif defined(__aarch64__)
int BigWhite_process_vm_readv_syscall = 270;
int BigWhite_process_vm_writev_syscall = 271;
#elif defined(__i386__)
int BigWhite_process_vm_readv_syscall = 347;
int BigWhite_process_vm_writev_syscall = 348;
#else
int BigWhite_process_vm_readv_syscall = 310;
int BigWhite_process_vm_writev_syscall = 311;
#endif

ssize_t BigWhite_process_v(pid_t __pid, const struct iovec *__local_iov, unsigned long __local_iov_count,
                           const struct iovec *__remote_iov, unsigned long __remote_iov_count,
                           unsigned long __flags, bool iswrite)
{
    return syscall((iswrite ? BigWhite_process_vm_writev_syscall : BigWhite_process_vm_readv_syscall), __pid,
                   __local_iov, __local_iov_count, __remote_iov, __remote_iov_count, __flags);
}

int BigWhite_getProcessID(const char *packageName)
{
    int id = -1;
    DIR *dir;
    FILE *fp;
    char filename[64];
    char cmdline[64];
    struct dirent *entry;
    dir = opendir("/proc");
    while ((entry = readdir(dir)) != NULL)
    {
        id = atoi(entry->d_name);
        if (id != 0)
        {
            sprintf(filename, "/proc/%d/cmdline", id);
            fp = fopen(filename, "r");
            if (fp)
            {
                fgets(cmdline, sizeof(cmdline), fp);
                fclose(fp);
                if (strcmp(packageName, cmdline) == 0)
                {
                    return id;
                }
            }
        }
    }
    closedir(dir);
    return -1;
}

bool BigWhite_pvm(int BigWhitePid, void *address, void *buffer, size_t size, bool iswrite)
{
    struct iovec local[1];
    struct iovec remote[1];
    local[0].iov_base = buffer;
    local[0].iov_len = size;
    remote[0].iov_base = address;
    remote[0].iov_len = size;
    if (BigWhitePid < 0)
    {
        return false;
    }
    ssize_t bytes = BigWhite_process_v(BigWhitePid, local, 1, remote, 1, 0, iswrite);
    return bytes == size;
}

bool BigWhite_vm_readv(int BigWhitePid, unsigned long address, void *buffer, size_t size)
{
    return BigWhite_pvm(BigWhitePid, reinterpret_cast<void *>(address), buffer, size, false);
}

// 添加pwrite64写入函数
bool BigWhite_pwrite64(int BigWhitePid, unsigned long address, void *buffer, size_t size) {
    char filename[64];
    snprintf(filename, sizeof(filename), "/proc/%d/mem", BigWhitePid);
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        return false;
    }
    
    ssize_t bytes = pwrite64(fd, buffer, size, address);
    close(fd);
    return bytes == size;
}

// 修改内存写入函数
bool BigWhite_vm_writev(int BigWhitePid, unsigned long address, void *buffer, size_t size) {
    // 首先尝试使用process_vm_writev
    struct iovec local[1];
    struct iovec remote[1];
    local[0].iov_base = buffer;
    local[0].iov_len = size;
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = size;
    
    ssize_t bytes = BigWhite_process_v(BigWhitePid, local, 1, remote, 1, 0, true);
    if (bytes == size) {
        return true;
    }
    
    // 如果process_vm_writev失败，尝试使用pwrite64
    return BigWhite_pwrite64(BigWhitePid, address, buffer, size);
}

struct AddressData
{
    long *addrs;
    int count;
};
struct MemPage
{
    long start;
    long end;
    char flags[8];
    char name[128];
    void *buf = NULL;
};

// 支持的搜索类型
enum
{
    DWORD,
    FLOAT,
    BYTE,
    WORD,
    QWORD,
    XOR,
    DOUBLE,
};

// 支持的内存范围(请参考GG修改器内存范围)
enum
{
    Mem_Auto, // 所以内存页
    Mem_A,
    Mem_Ca,
    Mem_Cd,
    Mem_Cb,
    Mem_Jh,
    Mem_J,
    Mem_S,
    Mem_V,
    Mem_Xa,
    Mem_Xs,
    Mem_As,
    Mem_B,
    Mem_O,
};

int memContrast(char *str)
{
    if (strlen(str) == 0)
        return Mem_A;
    if (strstr(str, "/dev/ashmem/") != NULL)
        return Mem_As;
    if (strstr(str, "/system/fonts/") != NULL)
        return Mem_B;
    if (strstr(str, "/data/app/") != NULL)
        return Mem_Xa;
    if (strstr(str, "/system/framework/") != NULL)
        return Mem_Xs;
    if (strcmp(str, "[anon:libc_malloc]") == 0)
        return Mem_Ca;
    if (strstr(str, ":bss") != NULL)
        return Mem_Cb;
    if (strstr(str, "/data/data/") != NULL)
        return Mem_Cd;
    if (strstr(str, "[anon:dalvik") != NULL)
        return Mem_J;
    if (strcmp(str, "[stack]") == 0)
        return Mem_S;
    if (strcmp(str, "/dev/kgsl-3d0") == 0)
        return Mem_V;
    return Mem_O;
}
// 根据类型判断类型所占字节大小
size_t judgSize(int type)
{
    switch (type)
    {
    case DWORD:
    case FLOAT:
    case XOR:
        return 4;
    case BYTE:
        return sizeof(char);
    case WORD:
        return sizeof(short);
    case QWORD:
        return sizeof(long);
    case DOUBLE:
        return sizeof(double);
    }
    return 4;
}
template <typename T>
AddressData search(int BigWhitePid, T value, int type, int mem, bool debug)
{
    size_t size = judgSize(type);
    MemPage *mp = NULL;
    AddressData ad;
    long *tmp, *ret = NULL;
    int count = 0;
    char filename[32];
    char line[1024];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", BigWhitePid);
    FILE *fp = fopen(filename, "r");
    if (fp != NULL)
    {
        tmp = (long *)calloc(1024000, sizeof(long));
        while (fgets(line, sizeof(line), fp))
        {
            mp = (MemPage *)calloc(1, sizeof(MemPage));
            sscanf(line, "%p-%p %s %*p %*p:%*p %*p   %[^\n]%s", &mp->start, &mp->end,
                   mp->flags, mp->name);
            if ((memContrast(mp->name) == mem || mem == Mem_Auto) && strstr(mp->flags, "r") != NULL)
            {
                mp->buf = malloc(mp->end - mp->start);
                BigWhite_vm_readv(BigWhitePid, mp->start, mp->buf, mp->end - mp->start);
                for (int i = 0; i < (mp->end - mp->start) / size; i++)
                {
                    // 在进行指针算术运算之前，将 void* 指针转换为正确的类型指针
                    if ((type == XOR ? (*(int *)((char *)mp->buf + i * size) ^ mp->start + i * size)
                                     : *(T *)((char *)mp->buf + i * size)) == value)
                    {
                        *(tmp + count) = mp->start + i * size;
                        count++;
                        if (debug)
                        {
                            std::cout << "index:" << count
                                      << "    value:" << (type == XOR ? *(int *)((char *)mp->buf + i * size) ^ (mp->start + i * size) : *(T *)((char *)mp->buf + i * size));
                            printf("    address:%p\n", mp->start + i * size);
                        }
                    }
                }
                free(mp->buf);
            }
            free(mp); // 在循环结束后释放 mp 指针
        }
        fclose(fp);
    }
    if (debug)
        printf("搜索结束，共%d条结果\n", count);
    ret = (long *)calloc(count, sizeof(long));
    memcpy(ret, tmp, count * (sizeof(long)));
    free(tmp);
    ad.addrs = ret;
    ad.count = count;
    return ad;
}

// 定义搜索条件结构体
struct SearchCondition {
    union {
        int i_value;
        float f_value;
        double d_value;
        char b_value;
        short w_value;
        long long q_value;
    } value;
    int type;            // 数据类型
    unsigned long offset; // 偏移量
};

// 定义搜索结果结构体
struct SearchResult {
    long* addresses;     // 地址数组
    int count;          // 结果数量
};

// 带偏移量的多级内存搜索函数
template<typename T>
SearchResult searchWithOffset(int BigWhitePid, const SearchCondition* conditions, int conditionCount, int mem, bool debug) {
    SearchResult result;
    result.addresses = nullptr;
    result.count = 0;
    
    // 第一次搜索，使用第一个条件
    AddressData firstResult;
    switch(conditions[0].type) {
        case DWORD:
            firstResult = search<int>(BigWhitePid, conditions[0].value.i_value, DWORD, mem, debug);
            break;
        case FLOAT:
            firstResult = search<float>(BigWhitePid, conditions[0].value.f_value, FLOAT, mem, debug);
            break;
        case BYTE:
            firstResult = search<char>(BigWhitePid, conditions[0].value.b_value, BYTE, mem, debug);
            break;
        case WORD:
            firstResult = search<short>(BigWhitePid, conditions[0].value.w_value, WORD, mem, debug);
            break;
        case QWORD:
            firstResult = search<long long>(BigWhitePid, conditions[0].value.q_value, QWORD, mem, debug);
            break;
        case DOUBLE:
            firstResult = search<double>(BigWhitePid, conditions[0].value.d_value, DOUBLE, mem, debug);
            break;
        default:
            return result;
    }
    
    if (firstResult.count == 0 || !firstResult.addrs) {
        return result;
    }
    
    // 预分配最大可能的结果空间
    long* validAddresses = (long*)calloc(firstResult.count, sizeof(long));
    if (!validAddresses) {
        free(firstResult.addrs);
        return result;
    }
    
    int validCount = 0;
    
    // 遍历第一次搜索结果
    for (int i = 0; i < firstResult.count; i++) {
        long baseAddr = firstResult.addrs[i];
        if (!baseAddr) continue;  // 跳过无效地址
        
        bool match = true;
        
        // 检查后续条件
        for (int j = 1; j < conditionCount; j++) {
            long checkAddr = baseAddr + conditions[j].offset;
            bool valueMatch = false;
            
            // 根据类型读取和比较值
            switch(conditions[j].type) {
                case DWORD: {
                    int value;
                    if (BigWhite_vm_readv(BigWhitePid, checkAddr, &value, sizeof(int))) {
                        valueMatch = (value == conditions[j].value.i_value);
                    }
                    break;
                }
                case FLOAT: {
                    float value;
                    if (BigWhite_vm_readv(BigWhitePid, checkAddr, &value, sizeof(float))) {
                        valueMatch = (value == conditions[j].value.f_value);
                    }
                    break;
                }
                case BYTE: {
                    char value;
                    if (BigWhite_vm_readv(BigWhitePid, checkAddr, &value, sizeof(char))) {
                        valueMatch = (value == conditions[j].value.b_value);
                    }
                    break;
                }
                case WORD: {
                    short value;
                    if (BigWhite_vm_readv(BigWhitePid, checkAddr, &value, sizeof(short))) {
                        valueMatch = (value == conditions[j].value.w_value);
                    }
                    break;
                }
                case QWORD: {
                    long long value;
                    if (BigWhite_vm_readv(BigWhitePid, checkAddr, &value, sizeof(long long))) {
                        valueMatch = (value == conditions[j].value.q_value);
                    }
                    break;
                }
                case DOUBLE: {
                    double value;
                    if (BigWhite_vm_readv(BigWhitePid, checkAddr, &value, sizeof(double))) {
                        valueMatch = (value == conditions[j].value.d_value);
                    }
                    break;
                }
            }
            
            if (!valueMatch) {
                match = false;
                break;
            }
        }
        
        // 如果所有条件都匹配，保存有效地址
        if (match) {
            validAddresses[validCount++] = baseAddr;
        }
    }
    
    // 如果找到有效结果
    if (validCount > 0) {
        // 分配最终结果空间
        result.addresses = (long*)calloc(validCount, sizeof(long));
        if (result.addresses) {
            // 复制有效地址
            memcpy(result.addresses, validAddresses, validCount * sizeof(long));
            result.count = validCount;
        }
    }
    
    // 清理临时数组
    free(validAddresses);
    free(firstResult.addrs);
    
    return result;
}

struct Pattern {
    unsigned char* bytes;    // 特征码字节数组
    char* mask;             // 掩码数组
    int length;             // 特征码长度
};

AddressData searchPattern(int BigWhitePid, const Pattern* pattern, int mem)
{
    AddressData result;
    result.addrs = nullptr;
    result.count = 0;
    
    MemPage* mp = nullptr;
    long* tmp = (long*)calloc(1024000, sizeof(long));
    int count = 0;
    
    char filename[32];
    char line[1024];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", BigWhitePid);
    FILE* fp = fopen(filename, "r");
    
    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp)) {
            mp = (MemPage*)calloc(1, sizeof(MemPage));
            sscanf(line, "%p-%p %s %*p %*p:%*p %*p   %[^\n]%s", &mp->start, &mp->end,
                   mp->flags, mp->name);
                   
            if ((memContrast(mp->name) == mem || mem == Mem_Auto) && strstr(mp->flags, "r") != NULL) {
                mp->buf = malloc(mp->end - mp->start);
                if (BigWhite_vm_readv(BigWhitePid, mp->start, mp->buf, mp->end - mp->start)) {
                    unsigned char* data = (unsigned char*)mp->buf;
                    size_t dataSize = mp->end - mp->start;
                    
                    for (size_t i = 0; i < dataSize - pattern->length; i++) {
                        bool match = true;
                        for (int j = 0; j < pattern->length; j++) {
                            if (pattern->mask[j] == 'x' && data[i + j] != pattern->bytes[j]) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            tmp[count++] = mp->start + i;
                        }
                    }
                }
                free(mp->buf);
            }
            free(mp);
        }
        fclose(fp);
    }
    
    if (count > 0) {
        result.addrs = (long*)calloc(count, sizeof(long));
        memcpy(result.addrs, tmp, count * sizeof(long));
        result.count = count;
    }
    
    free(tmp);
    return result;
}

extern "C"
{
    EXPORT int BigWhite_GetPID(const char *packageName)
    {
        DIR *dir = NULL;
        struct dirent *ptr = NULL;
        FILE *fp = NULL;
        char filepath[1024];
        char filetext[128];
        dir = opendir("/proc");
        if (NULL != dir)
        {
            while ((ptr = readdir(dir)) != NULL)
            {
                if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
                    continue;
                if (ptr->d_type != DT_DIR)
                    continue;
                sprintf(filepath, "/proc/%s/cmdline", ptr->d_name);
                fp = fopen(filepath, "r");
                if (NULL != fp)
                {
                    fgets(filetext, sizeof(filetext), fp);
                    if (strcmp(filetext, packageName) == 0)
                    {
                        break;
                    }
                    fclose(fp);
                }
            }
        }
        if (readdir(dir) == NULL)
        {
            return 0;
        }
        closedir(dir);
        return atoi(ptr->d_name);
    }
    EXPORT int BigWhite_GetPID2(const char *packageName)
    {
        DIR *dir = NULL;
        struct dirent *ptr = NULL;
        FILE *fp = NULL;
        char filepath[1024];
        char filetext[128];
        int lastPID = 0; // 用于记录最后一个匹配到的 PID
        dir = opendir("/proc");
        if (NULL != dir)
        {
            while ((ptr = readdir(dir)) != NULL)
            {
                if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
                    continue;
                if (ptr->d_type != DT_DIR)
                    continue;
                sprintf(filepath, "/proc/%s/cmdline", ptr->d_name);
                fp = fopen(filepath, "r");
                if (NULL != fp)
                {
                    fgets(filetext, sizeof(filetext), fp);
                    if (strcmp(filetext, packageName) == 0)
                    {
                        lastPID = atoi(ptr->d_name); // 更新最后一个匹配到的 PID
                    }
                    fclose(fp);
                }
            }
        }
        closedir(dir);
        return lastPID;
    }
    EXPORT unsigned long BigWhite_GetModuleBase(int pid, const char *module_name)
    {
        FILE *fp;
        unsigned long addr = 0;
        char *pch;
        char filename[64];
        char line[1024];
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
        fp = fopen(filename, "r");
        if (fp != NULL)
        {
            while (fgets(line, sizeof(line), fp))
            {
                if (strstr(line, module_name))
                {
                    pch = strtok(line, "-");
                    addr = strtoul(pch, NULL, 16);
                    if (addr == 0x8000)
                        addr = 0;
                    break;
                }
            }
            fclose(fp);
        }
        return addr;
    }
    EXPORT unsigned long BigWhite_GetPtr64(int BigWhitePid, unsigned long addr)
    {
        unsigned long var = 0;
        BigWhite_vm_readv(BigWhitePid, addr, &var, 8);
        return (var);
    }
    EXPORT unsigned long BigWhite_GetPtr32(int BigWhitePid, unsigned long addr)
    {
        unsigned long var = 0;
        BigWhite_vm_readv(BigWhitePid, addr, &var, 4);
        return (var);
    }
    EXPORT float BigWhite_GetFloat(int BigWhitePid, unsigned long addr)
    {
        float var = 0;
        BigWhite_vm_readv(BigWhitePid, addr, &var, 4);
        return (var);
    }
    EXPORT int BigWhite_GetDword(int BigWhitePid, unsigned long addr)
    {
        int var = 0;
        BigWhite_vm_readv(BigWhitePid, addr, &var, 4);
        return (var);
    }
    EXPORT bool BigWhite_WriteDword(int BigWhitePid, unsigned long addr, int data)
    {
        return BigWhite_vm_writev(BigWhitePid, addr, &data, 4);
    }
    EXPORT bool BigWhite_WriteFloat(int BigWhitePid, unsigned long addr, float data)
    {
        return BigWhite_vm_writev(BigWhitePid, addr, &data, 4);
    }
    EXPORT void BigWhite_GetUTF8(int BigWhitePid, UTF8 *buf, unsigned long namepy)
    {
        UTF16 buf16[16] = {0};
        BigWhite_vm_readv(BigWhitePid, namepy, buf16, 28);
        UTF16 *pTempUTF16 = buf16;
        UTF8 *pTempUTF8 = buf;
        UTF8 *pUTF8End = pTempUTF8 + 32;
        while (pTempUTF16 < pTempUTF16 + 28)
        {
            if (*pTempUTF16 <= 0x007F && pTempUTF8 + 1 < pUTF8End)
            {
                *pTempUTF8++ = (UTF8)*pTempUTF16;
            }
            else if (*pTempUTF16 >= 0x0080 && *pTempUTF16 <= 0x07FF && pTempUTF8 + 2 < pUTF8End)
            {
                *pTempUTF8++ = (*pTempUTF16 >> 6) | 0xC0;
                *pTempUTF8++ = (*pTempUTF16 & 0x3F) | 0x80;
            }
            else if (*pTempUTF16 >= 0x0800 && *pTempUTF16 <= 0xFFFF && pTempUTF8 + 3 < pUTF8End)
            {
                *pTempUTF8++ = (*pTempUTF16 >> 12) | 0xE0;
                *pTempUTF8++ = ((*pTempUTF16 >> 6) & 0x3F) | 0x80;
                *pTempUTF8++ = (*pTempUTF16 & 0x3F) | 0x80;
            }
            else
            {
                break;
            }
            pTempUTF16++;
        }
    }
    EXPORT AddressData Search_DWORD(int BigWhitePid, int value, int mem)
    {
        return search<int>(BigWhitePid, value, DWORD, mem, false);
    }
    EXPORT AddressData Search_FLOAT(int BigWhitePid, float value, int mem)
    {
        return search<float>(BigWhitePid, value, FLOAT, mem, false);
    }
    EXPORT AddressData Search_BYTE(int BigWhitePid, char value, int mem)
    {
        return search<char>(BigWhitePid, value, BYTE, mem, false);
    }
    EXPORT AddressData Search_WORD(int BigWhitePid, short value, int mem)
    {
        return search<short>(BigWhitePid, value, WORD, mem, false);
    }
    EXPORT AddressData Search_QWORD(int BigWhitePid, long long value, int mem)
    {
        return search<long long>(BigWhitePid, value, QWORD, mem, false);
    }
    EXPORT AddressData Search_XOR(int BigWhitePid, int value, int mem)
    {
        return search<int>(BigWhitePid, value, XOR, mem, false);
    }
    EXPORT AddressData Search_DOUBLE(int BigWhitePid, double value, int mem)
    {
        return search<double>(BigWhitePid, value, DOUBLE, mem, false);
    }
    EXPORT SearchResult BigWhite_SearchWithOffset(int BigWhitePid, const SearchCondition* conditions, int conditionCount, int mem) {
        return searchWithOffset<int>(BigWhitePid, conditions, conditionCount, mem, false);
    }
    EXPORT void BigWhite_FreeSearchResult(SearchResult* result) {
        if (result && result->addresses) {
            free(result->addresses);
            result->addresses = nullptr;
            result->count = 0;
        }
    }
    EXPORT AddressData BigWhite_SearchPattern(int BigWhitePid, const unsigned char* pattern, const char* mask, int mem) {
        Pattern p;
        p.length = strlen(mask);
        p.bytes = (unsigned char*)malloc(p.length);
        p.mask = (char*)malloc(p.length + 1);
        
        memcpy(p.bytes, pattern, p.length);
        strcpy(p.mask, mask);
        
        AddressData result = searchPattern(BigWhitePid, &p, mem);
        
        free(p.bytes);
        free(p.mask);
        
        return result;
    }
