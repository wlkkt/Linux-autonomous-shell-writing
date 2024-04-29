#define _XOPEN_SOURCE//���ڴ���putenv����ȱ������������,�ұ��붨����stdlib.h֮ǰ
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>


#define SIZE 512 //����������ַ���
#define NUM 32  //������ָ������������ָ����
#define TAG " " //��ʶ��
#define ZERO '\0' //����˫�ػس�����
#define SkipPath(p) do{p += strlen(p)-1; while(*p != '/') p--;}while(0)//ѭ��������ֻ��ӡ����·��������ֵ
//�ú���������Ǵ���ָ���ָ��,ԭ����ָ��ָ��һ���ַ���,��������ֻ����ָ���������ҵ�һ���Ӵ�

char cwd[SIZE * 2];//�������� 
int lastcode = 0;//������
char* Argv[NUM];//�������ַ�������

//��ȡ��ǰ�û����ӿ�
const char* GetUserName()
{
    const char* name = getenv("USER");
    if (name == 0) return "None";
    return name;
}


//��ȡ��ǰ�������ӿ�
const char* GetHostName()
{
    const char* hostname = getenv("HOSTNAME");
    if (hostname == 0) return "None";
    return hostname;
}


//��ȡ��ǰ����·���ӿ�
const char* GetCwd()
{
    const char* cwd = getenv("PWD");
    if (cwd == 0) return "None";
    return cwd;
}


//��������в���ӡ
void MakeCommandLineAndPrint()
{
    char line[SIZE];
    const char* username = GetUserName();
    const char* hostname = GetHostName();
    const char* cwd = GetCwd();


    //�Ը�ʽ������ʽ��������������д��
    SkipPath(cwd);//ָ���ȡ��ǰ����·�������Ҳ���Ӵ�����
    snprintf(line, sizeof(line), "[%s@%s %s]> ", username, hostname, strlen(cwd) == 1 ? "/" : cwd + 1);
    //����Ӵ�����Ϊ1,��cwd��\,�����Ϊ1���ӡ/�������
    printf("%s", line);
    fflush(stdout);
    printf("\n");
}

//��ȡ�������ַ����ӿ�
int GetUserCommand(char usercommand[], size_t n)
{
    char* str = fgets(usercommand, n, stdin);
    if (str == NULL)return -1;
    usercommand[strlen(usercommand) - 1] = ZERO;//�����������ַ���ʱ�������һ��\n,���ַ���+'\n'+'\0',��ᵼ�¶��һ��,���������ڻ�ȡ��ȫ���ַ�����ֱ���ڸ��ַ��������'\0'���ǵ���ȡ����'\n'
    return strlen(usercommand);
}

//�ָ��ַ����ӿ�
void SplitCommand(char usercommand[], size_t n)
{
    Argv[0] = strtok(usercommand, TAG);
    int index = 1;
    while ((Argv[index++] = strtok(NULL, TAG)));//ѭ����ֵ���ж�
}

//ִ������ӿ�
void ExecuteCommand()
{
    pid_t id = fork();
    if (id == 0)
    {
        //child
        execvp(Argv[0], Argv);//(Ҫִ�еĳ���,��ôִ�иó���)
        exit(errno);//execvp�����滻ʧ�ܺ�᷵��һ�������룬�˳����˳�����Ǹô�����
    }
    else
    {
        //father
        int status = 0;
        pid_t rid = waitpid(id, &status, 0);//�����������ȴ�
        if (rid > 0)
        {
            lastcode = WEXITSTATUS(status);//�ӽ��̵��˳���lastcode
            if (lastcode != 0)printf("%s:%s:%d\n", Argv[0], strerror(lastcode), lastcode);
        }
    }
}

//��ȡ��Ŀ¼�ӿ�
const char* GetHome()
{
    const char* home = getenv("HOME");
    if (home == NULL) return "/";//�Ҳ����ͷŵ���Ŀ¼��
    return home;//��Ŀ¼��Ϊ���򷵻ؼ�Ŀ¼
}


//ִ���ڽ�����cd
void Cd()
{
    const char* path = Argv[1];//ѡ����ôִ��cd����
    if (path == NULL)//���ֻ��һ��������cdָ��,��path��ż�Ŀ¼��Ϣ
        path = GetHome();
    chdir(path);//���ĵ�ǰ����Ŀ¼

    //ˢ�»�������(��ˢ��[]����һ������)
    char temp[SIZE * 2];//��ʱ������
    getcwd(temp, sizeof(temp));//��ȡ��ǰ����Ŀ¼�ľ���·��
    snprintf(cwd, sizeof(cwd), "PWD=%s", temp);
    putenv(cwd);//�򻷾���������д��
}

//����Ƿ����ڽ�ָ��
int CheckBuildin()
{
    int yes = 0;
    const char* enter_cmd = Argv[0];//ָ�������д�ŵ�Ҫִ�еĳ�����
    if (strcmp(enter_cmd, "cd") == 0)
    {
        yes = 1;
        Cd();//���ڽ������ִ��
    }
    else if (strcmp(enter_cmd, "echo") == 0 && strcmp(Argv[1], "$?") == 0)
    {
        yes = 1;
        printf("%d\n", lastcode);//��ӡ�˳���
        lastcode = 0;
    }
    return yes;//�����жϽ��
}


int main()
{
    int quit = 0;
    while (quit == 0)//���ִ������
    {
        //1����ȡ�����в���ӡ
        MakeCommandLineAndPrint();

        //2����������ȡ�ַ���
        char usercommand[SIZE];
        int n = GetUserCommand(usercommand, sizeof(usercommand));
        if (n <= 0) return 1;//��ȡ�ַ���ʧ�ܻ��߻�ȡ�ַ�������Ϊ0ʱ�˳����˳���Ϊ1

        //3���ָ��������ַ���
        SplitCommand(usercommand, sizeof(usercommand));

        //4������Ƿ����ڽ�����
        n = CheckBuildin();//�����ָ����ɸ�����ִ�е�
        if (n) continue;//������ڽ�������ڼ�⺯����ִ�����ڽ�Ŀ����󷵻�ѭ����ʼ����������ִ����ͨ����

        //5��ִ������
        ExecuteCommand();//��ʱ�Żᴴ���ӽ���
    }
}
