#define _XOPEN_SOURCE//用于处理putenv函数缺少声明的问题,且必须定义在stdlib.h之前
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>


#define SIZE 512 //命令行最多字符数
#define NUM 32  //命令行指令数组中最多的指令数
#define TAG " " //标识符
#define ZERO '\0' //处理双重回车问题
#define SkipPath(p) do{p += strlen(p)-1; while(*p != '/') p--;}while(0)//循环遍历到只打印工作路径的最右值
//该函数处理的是传入指针的指向,原来该指针指向一大串字符串,现在我们只让它指向其中最右的一个子串

char cwd[SIZE * 2];//环境变量 
int lastcode = 0;//错误码
char* Argv[NUM];//命令行字符串数组

//获取当前用户名接口
const char* GetUserName()
{
    const char* name = getenv("USER");
    if (name == 0) return "None";
    return name;
}


//获取当前主机名接口
const char* GetHostName()
{
    const char* hostname = getenv("HOSTNAME");
    if (hostname == 0) return "None";
    return hostname;
}


//获取当前工作路径接口
const char* GetCwd()
{
    const char* cwd = getenv("PWD");
    if (cwd == 0) return "None";
    return cwd;
}


//组合命令行并打印
void MakeCommandLineAndPrint()
{
    char line[SIZE];
    const char* username = GetUserName();
    const char* hostname = GetHostName();
    const char* cwd = GetCwd();


    //以格式化的形式向命令行数组中写入
    SkipPath(cwd);//指向获取当前工作路径的最右侧的子串内容
    snprintf(line, sizeof(line), "[%s@%s %s]> ", username, hostname, strlen(cwd) == 1 ? "/" : cwd + 1);
    //如果子串长度为1,则cwd是\,如果不为1则打印/后的内容
    printf("%s", line);
    fflush(stdout);
    printf("\n");
}

//获取命令行字符串接口
int GetUserCommand(char usercommand[], size_t n)
{
    char* str = fgets(usercommand, n, stdin);
    if (str == NULL)return -1;
    usercommand[strlen(usercommand) - 1] = ZERO;//输入命令行字符串时会多输入一个\n,即字符串+'\n'+'\0',这会导致多打一行,现在我们在获取完全部字符串后直接在该字符串后加上'\0'覆盖掉读取到的'\n'
    return strlen(usercommand);
}

//分割字符串接口
void SplitCommand(char usercommand[], size_t n)
{
    Argv[0] = strtok(usercommand, TAG);
    int index = 1;
    while ((Argv[index++] = strtok(NULL, TAG)));//循环赋值并判断
}

//执行命令接口
void ExecuteCommand()
{
    pid_t id = fork();
    if (id == 0)
    {
        //child
        execvp(Argv[0], Argv);//(要执行的程序,怎么执行该程序)
        exit(errno);//execvp函数替换失败后会返回一个错误码，退出的退出码就是该错误码
    }
    else
    {
        //father
        int status = 0;
        pid_t rid = waitpid(id, &status, 0);//父进程阻塞等待
        if (rid > 0)
        {
            lastcode = WEXITSTATUS(status);//子进程的退出码lastcode
            if (lastcode != 0)printf("%s:%s:%d\n", Argv[0], strerror(lastcode), lastcode);
        }
    }
}

//获取家目录接口
const char* GetHome()
{
    const char* home = getenv("HOME");
    if (home == NULL) return "/";//找不到就放到根目录下
    return home;//根目录不为空则返回家目录
}


//执行内建命令cd
void Cd()
{
    const char* path = Argv[1];//选择怎么执行cd程序
    if (path == NULL)//如果只有一个单独的cd指令,则path存放家目录信息
        path = GetHome();
    chdir(path);//更改当前工作目录

    //刷新环境变量(即刷新[]最后的一个内容)
    char temp[SIZE * 2];//临时缓冲区
    getcwd(temp, sizeof(temp));//获取当前工作目录的绝对路径
    snprintf(cwd, sizeof(cwd), "PWD=%s", temp);
    putenv(cwd);//向环境变量表中写入
}

//检测是否是内建指令
int CheckBuildin()
{
    int yes = 0;
    const char* enter_cmd = Argv[0];//指向数组中存放的要执行的程序名
    if (strcmp(enter_cmd, "cd") == 0)
    {
        yes = 1;
        Cd();//是内建命令就执行
    }
    else if (strcmp(enter_cmd, "echo") == 0 && strcmp(Argv[1], "$?") == 0)
    {
        yes = 1;
        printf("%d\n", lastcode);//打印退出码
        lastcode = 0;
    }
    return yes;//返回判断结果
}


int main()
{
    int quit = 0;
    while (quit == 0)//多次执行命令
    {
        //1、获取命令行并打印
        MakeCommandLineAndPrint();

        //2、获命令行取字符串
        char usercommand[SIZE];
        int n = GetUserCommand(usercommand, sizeof(usercommand));
        if (n <= 0) return 1;//获取字符串失败或者获取字符串长度为0时退出，退出码为1

        //3、分割命令行字符串
        SplitCommand(usercommand, sizeof(usercommand));

        //4、检测是否是内建命令
        n = CheckBuildin();//这里的指令还是由父进程执行的
        if (n) continue;//如果是内建命令就在检测函数中执行完内建目命令后返回循环开始处，而不是执行普通命令

        //5、执行命令
        ExecuteCommand();//此时才会创建子进程
    }
}
