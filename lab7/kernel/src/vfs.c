#include "vfs.h"
#include "tmpfs.h"
#include "memory.h"
#include "string.h"
#include "uart1.h"
#include "initramfs.h"
#include "dev_uart.h"

struct mount *rootfs;
struct filesystem reg_fs[MAX_FS_REG];
struct file_operations reg_dev[MAX_DEV_REG];

// register the file system to the kernel.
int register_filesystem(struct filesystem *fs)
{
    for (int i = 0; i < MAX_FS_REG;i++)
    {
        if(!reg_fs[i].name)
        {
            reg_fs[i].name = fs->name;
            reg_fs[i].setup_mount = fs->setup_mount;
            return i;
        }
    }
    return -1;
}

int register_dev(struct file_operations *fo)
{
    for (int i = 0; i < MAX_FS_REG; i++)
    {
        if (!reg_dev[i].open)
        {
            reg_dev[i] = *fo;
            return i;
        }
    }
    return -1;
}

// find specify file system's reg_fs
struct filesystem* find_filesystem(const char* fs_name)
{
    for (int i = 0; i < MAX_FS_REG; i++)
    {
        if (strcmp(reg_fs[i].name,fs_name)==0)
        {
            return &reg_fs[i];
        }
    }
    return 0;
}

// e.x. vfs_open("/lll/ddd/ggg", O_CREAT, &testfilew);
int vfs_open(const char *pathname, int flags, struct file **target)
{
    // 1. Lookup pathname
    // 3. Create a new file if O_CREAT is specified in flags and vnode not found
    struct vnode *node;
    // if pathname not found and have set O_CREAT
    if (vfs_lookup(pathname, &node) != 0 && (flags & O_CREAT))
    {
        int last_slash_idx = 0;
        for (int i = 0; i < strlen(pathname); i++)
        {
            if(pathname[i]=='/')
            {
                last_slash_idx = i;
            }
        }

        char dirname[MAX_PATH_NAME+1];
        strcpy(dirname, pathname);
        dirname[last_slash_idx] = 0;
        // dirname = /lll/ddd
        if (vfs_lookup(dirname,&node)!=0)
        {
            uart_sendline("cannot ocreate no dir name\r\n");
            return -1;
        }
        // pathname+last_slash_idx+1 = ggg
        node->v_ops->create(node, &node, pathname+last_slash_idx+1);
        *target = kmalloc(sizeof(struct file));
        node->f_ops->open(node, target);
        (*target)->flags = flags;
        return 0;
    }
    else // 2. Create a new file handle for this vnode if found.
    {
        *target = kmalloc(sizeof(struct file));
        node->f_ops->open(node, target);
        (*target)->flags = flags;
        return 0;
    }

    // lookup error code shows if file exist or not or other error occurs
    // 4. Return error code if fails
    return -1;
}

int vfs_close(struct file *file)
{
    // 1. release the file handle
    // 2. Return error code if fails
    file->f_ops->close(file);
    return 0;
}

int vfs_write(struct file *file, const void *buf, size_t len)
{
    // 1. write len byte from buf to the opened file.
    // 2. return written size or error code if an error occurs.
    return file->f_ops->write(file,buf,len);
}

int vfs_read(struct file *file, void *buf, size_t len)
{
    // 1. read min(len, readable size) byte to buf from the opened file.
    // 2. block if nothing to read for FIFO type
    // 2. return read size or error code if an error occurs.
    return file->f_ops->read(file, buf, len);
}

// vfs_mkdir("/foo/bar");
int vfs_mkdir(const char *pathname)
{
    char dirname[MAX_PATH_NAME] = {};
    char newdirname[MAX_PATH_NAME] = {};

    int last_slash_idx = 0;
    for (int i = 0; i < strlen(pathname); i++)
    {
        if (pathname[i] == '/')
        {
            last_slash_idx = i;
        }
    }

    // dirname = /foo, newdirname = bar
    memcpy(dirname, pathname, last_slash_idx);
    strcpy(newdirname, pathname + last_slash_idx + 1);

    struct vnode *node;
    if(vfs_lookup(dirname,&node)==0)
    {
        node->v_ops->mkdir(node,&node,newdirname);
        return 0;
    }

    uart_sendline("vfs_mkdir cannot find pathname");
    return -1;
}

// vfs_mount("/lll/ddd", "tmpfs");
int vfs_mount(const char *target, const char *filesystem)
{
    struct vnode *dirnode;
    struct filesystem *fs = find_filesystem(filesystem);
    if(!fs)
    {
        uart_sendline("vfs_mount cannot find filesystem\r\n");
        return -1;
    }

    if(vfs_lookup(target, &dirnode)==-1)
    {
        uart_sendline("vfs_mount cannot find dir\r\n");
        return -1;
    }else
    {
        // malloc a mount space for /ddd
        dirnode->mount = kmalloc(sizeof(struct mount));
        // set fs's mount at /ddd
        fs->setup_mount(fs, dirnode->mount);
    }
    return 0;
}

// lookup specify pathname's vnode
// e.x. vfs_lookup("/foo/abc", &dir_node);
int vfs_lookup(const char *pathname, struct vnode **target)
{
    if(strlen(pathname)==0)
    {
        *target = rootfs->root;
        return 0;
    }

    // first set dirnode to "/"
    struct vnode *dirnode = rootfs->root;
    char component_name[FILE_NAME_MAX+1] = {};
    int c_idx = 0;
    for (int i = 1; i < strlen(pathname); i++)
    {
        if (pathname[i] == '/')
        {
            component_name[c_idx++] = 0;
            // use vnode operations to check 
            if (dirnode->v_ops->lookup(dirnode, &dirnode, component_name) != 0)return -1;
            // redirect to new mounted filesystem
            if (dirnode->mount)
            {
                dirnode = dirnode->mount->root;
            }
            c_idx = 0;
        }
        else
        {
            component_name[c_idx++] = pathname[i];
        }
    }

    // deal with last name
    component_name[c_idx++] = 0;
    if (dirnode->v_ops->lookup(dirnode, &dirnode, component_name) != 0)return -1;
    // redirect to new mounted filesystem
    if (dirnode->mount)
    {
        dirnode = dirnode->mount->root;
    }

    *target = dirnode;

    return 0;
}

int vfs_mknod(char* pathname, int id)
{
    struct file* f = kmalloc(sizeof(struct file));
    //create file
    vfs_open(pathname, O_CREAT, &f);
    f->vnode->f_ops = &reg_dev[id];
    vfs_close(f);
    return 0;
}

void init_rootfs()
{
    int idx = register_tmpfs(); // register tmpfs filesystem
    rootfs = kmalloc(sizeof(struct mount)); // malloc rootfs's mount structure
    reg_fs[idx].setup_mount(&reg_fs[idx], rootfs); // mount tmpfs on rootfs

    vfs_mkdir("/initramfs");
    register_initramfs();
    vfs_mount("/initramfs","initramfs");

    // for dev
    vfs_mkdir("/dev");
    int uart_id = init_dev_uart();
    vfs_mknod("/dev/uart", uart_id);
    
    vfs_test();

}

void vfs_test()
{
    // test read/write
    vfs_mkdir("/lll");
    vfs_mkdir("/lll/ddd");
    // test mount
    vfs_mount("/lll/ddd", "tmpfs");
    struct file* testfilew;
    struct file *testfiler;
    char testbufw[0x50] = "Guava is a common tropical fruit.";
    char testbufr[0x50] = {};
    vfs_open("/lll/ddd/ggg", O_CREAT, &testfilew);
    vfs_open("/lll/ddd/ggg", O_CREAT, &testfiler);
    vfs_write(testfilew, testbufw, 10);
    vfs_read(testfiler, testbufr, 10);
    uart_sendline("%s\n",testbufr);

    struct file *testfile_initramfs;
    vfs_open("/initramfs/file0.txt", O_CREAT, &testfile_initramfs);
    vfs_write(testfile_initramfs, testbufw, 20);
    vfs_read(testfile_initramfs, testbufr, 20);
    uart_sendline("%s\n", testbufr);
}

void initramfs_test()
{
    char testbufw[0x50] = "Guava is a common tropical fruit.";
    char testbufr[0x50] = {};

    struct file *testfile_initramfs;
    vfs_open("/initramfs/file0.txt", O_CREAT, &testfile_initramfs);
    vfs_write(testfile_initramfs, testbufw, 20);
    vfs_read(testfile_initramfs, testbufr, 20);
    uart_sendline("%s\n", testbufr);
}

char *get_absolute_path(char *path, char *curr_working_dir)
{
    //relative path
    /*
    path = docs/file.txt
    curr_working_dir = /home/user
    path = /home/user/docs/file.txt
    */
    if(path[0] != '/')
    {
        char tmp[MAX_PATH_NAME];
        strcpy(tmp, curr_working_dir);
        if(strcmp(curr_working_dir,"/")!=0)strcat(tmp, "/");
        strcat(tmp, path);
        strcpy(path, tmp);
    }

    // deal with '.', '..'
    char absolute_path[MAX_PATH_NAME+1] = {};
    int idx = 0;
    for (int i = 0; i < strlen(path); i++)
    {
        // meet /..
        if (path[i] == '/' && path[i+1] == '.' && path[i+2] == '.')
        {
            for (int j = idx; j >= 0;j--)
            {
                if(absolute_path[j] == '/')
                {
                    absolute_path[j] = 0;
                    idx = j;
                }
            }
            i += 2;
            continue;
        }

        // ignore /.
        if (path[i] == '/' && path[i+1] == '.')
        {
            i++;
            continue;
        }

        absolute_path[idx++] = path[i];
    }
    absolute_path[idx] = 0;

    return strcpy(path, absolute_path);
}