#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>       //register_chrdev()
#include <linux/slab.h>     //kmelloc()
#include <linux/uaccess.h>  //copy_to_user(), copy_from_user()
#include <linux/kfifo.h>    //kfifo()

#define DEV_MAJOR_NUMBER 275
#define DEV_NAME "BufferedMem"

/* init 할때 arg로 버퍼 크기 및 read 크기 설정 */
static int buffer_size = 32;        //default N 버퍼 사이즈
static int read_buffer_size = 3;    //default M 바이트 단위
module_param(buffer_size, int, 0000);   
MODULE_PARM_DESC(buffer_size, "buffer size (int) (kfifo) default 32");
module_param(read_buffer_size, int, 0000);
MODULE_PARM_DESC(read_buffer_size, "one read size (byte) default 3");

struct kfifo fifo_buffer; // 내부 버퍼

/* ioctl command 지정 0:내부 버퍼 크기 변경, 1: Read 버퍼 크기 변경 type int*/
#define CH_WRITE_BUFFER_SIZE _IOW(DEV_MAJOR_NUMBER,0,int)
#define CH_READ_BUFFER_SIZE _IOW(DEV_MAJOR_NUMBER,1,int)

static ssize_t BufferedMem_write(struct file *file, const char *buf, size_t length, loff_t *ofs)
{
    ssize_t retval; //write 상태 return 변수
    char * write_buffer;
    int i;
    char onec;

    printk(KERN_INFO "BufferedMem_write\n");

    /* user->kernel copy buffer */
    write_buffer= kmalloc(length, GFP_KERNEL);   

    if (!write_buffer){
        printk("error: allocating memory for the buffer\n");
        retval = -ENOMEM;
        goto out;
    }
    printk(KERN_INFO "kmalloc write_buffer\n");

    /* user->kernel copy*/
    retval = length;
    if (copy_from_user(write_buffer, buf, length)){
        retval = -EFAULT;
        goto out;   //실패 시 종료
    }

    for(i = 0; i<length;i++){
        // 버퍼 다 차면 앞에꺼 뺌.
        if(kfifo_is_full(&fifo_buffer)){
            kfifo_out(&fifo_buffer, &onec, sizeof(onec));
        }
        onec=write_buffer[i]; //유저에서 받은 값 내부버퍼에 입력
        kfifo_in(&fifo_buffer,&onec,sizeof(onec));  
        printk(KERN_INFO "fifo in : %c\n", onec);
    }
    printk(KERN_INFO "kfifo in ok\n");
    printk(KERN_INFO "kfifo size: %d, kfifo len : %d, kfifoavail : %d\n", 
        kfifo_size(&fifo_buffer),kfifo_len(&fifo_buffer), kfifo_avail(&fifo_buffer));
    printk(KERN_INFO "write ok\n");
out:
    if (write_buffer)
    {
        kfree(write_buffer); // 버퍼 반납
        printk(KERN_INFO "free write_buffer\n");
    }
    return retval;
}

static ssize_t BufferedMem_read(struct file *file, char *buf, size_t length, loff_t *ofs)
{
    ssize_t retval;
    char val;
    char* read_buffer;
    int i;

    printk(KERN_INFO "BufferedMem_read!\n");
    if(*ofs > 0){
        retval = 0;
        goto out;
    }

    /* kernel->user 로 보내기 위한 buffer */
    read_buffer= kmalloc(read_buffer_size, GFP_KERNEL);
    printk(KERN_INFO "kmalloc read_buffer\n");
    if (!read_buffer){
        printk("error: allocating memory for the buffer\n");
        retval = -ENOMEM;
        goto out;
    }
    memset(read_buffer, 0, read_buffer_size); //버퍼 0으로 초기화

    /* 내부 버퍼(kfifo) 비어있으면 종료 */
    if(kfifo_is_empty(&fifo_buffer)){
        retval = 0;
        printk("empty kfifo");
        goto out;
    }

    /* 내부버퍼에서 read_buffer_size 만큼 빼서 저장 */
    
    for(i=0;i<read_buffer_size;i++){
        if(kfifo_is_empty(&fifo_buffer)){
            break;
        }
        kfifo_out(&fifo_buffer, &val, sizeof(val));
        read_buffer[i] = val;
        printk(KERN_INFO "fifo out: %c\n", val);
    }
    printk(KERN_INFO "kfifo out ok\n");
    printk(KERN_INFO "kfifo size: %d, kfifo len : %d, kfifoavail : %d\n", 
        kfifo_size(&fifo_buffer),kfifo_len(&fifo_buffer), kfifo_avail(&fifo_buffer));

    /* kernel->user copy */
    retval = read_buffer_size;
    if (copy_to_user(buf, read_buffer, read_buffer_size)){
        retval = -EFAULT;
        goto out;
    }
    printk(KERN_INFO "read ok\n");

out:
    if (read_buffer){
        kfree(read_buffer); // 버퍼 반납
        printk(KERN_INFO "free read_buffer\n");
    }
    *ofs += retval;
    return retval;
}

static long BufferedMem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int new_buf_size;   //입력된 새로운 size
    char onec;
    struct kfifo tmp_fifo_buffer;   //내부 버퍼 크기 수정 시 데이터 이동을 위해 임시 버퍼 할당

    switch (cmd){
        /* 내부 버퍼 크기 수정 */
        case CH_WRITE_BUFFER_SIZE:  
            printk(KERN_INFO "ch_write_buffer_size\n");
            /*ioctl user->kernel data copy*/
            if(copy_from_user(&new_buf_size,(void __user *)arg,sizeof(new_buf_size))){
                printk(KERN_INFO "error\n");
                return -EFAULT;
            }
            
            
            if (kfifo_alloc(&tmp_fifo_buffer, buffer_size, GFP_KERNEL)){
                printk(KERN_ERR "error kfifo_alloc\n");
            }
            printk(KERN_INFO "kfifo tmp_fifo_buffer\n");
            while (!kfifo_is_empty(&fifo_buffer)){  //기존 버퍼 데이터 임시 저장
                kfifo_out(&fifo_buffer, &onec, sizeof(onec));
                kfifo_in(&tmp_fifo_buffer,&onec,sizeof(onec));
            }
            
            /* 내부 버퍼 Size 변경 */
            buffer_size = new_buf_size;     //버퍼 사이즈 갱신
            kfifo_free(&fifo_buffer);       //기존 버퍼 반납
            kfifo_alloc(&fifo_buffer, buffer_size, GFP_KERNEL); //새로운 크기버퍼 할당
            while (!kfifo_is_empty(&tmp_fifo_buffer))
            {
                if(kfifo_is_full(&fifo_buffer)){    //크기가 줄어들었을땐 앞에 내용 지움
                    kfifo_out(&fifo_buffer, &onec, sizeof(onec));
                }
                kfifo_out(&tmp_fifo_buffer, &onec, sizeof(onec));
                kfifo_in(&fifo_buffer,&onec,sizeof(onec));
            }
            
            kfifo_free(&tmp_fifo_buffer);   //임시버퍼 반납
            printk(KERN_INFO "free tmp_fifo_buffer\n"); 

            new_buf_size = kfifo_size(&fifo_buffer);    //실제 변경된 버퍼 크기 유저에게 제공
            printk(KERN_INFO "success modify buffer, input_size : %d \n", buffer_size);
            printk(KERN_INFO "kfifo size: %d, kfifo len : %d, kfifoavail : %d\n", 
                kfifo_size(&fifo_buffer),kfifo_len(&fifo_buffer), kfifo_avail(&fifo_buffer));
            
            /* kernel->user copy*/
            if (copy_to_user((void __user *)arg, &new_buf_size, sizeof(new_buf_size))){
                printk(KERN_INFO "error\n");
                return -EFAULT;
            }
            
            break;

        /* Read 버퍼 크기 수정 */
        case CH_READ_BUFFER_SIZE:
            printk(KERN_INFO "ch_read_buffer_size\n");
            /* user->kernel data copy (int)*/
            if(copy_from_user(&new_buf_size,(void __user *)arg,sizeof(new_buf_size))){
                printk(KERN_INFO "error\n");
                return -EFAULT;
            }
            /* Read buffer size modfiy */
            read_buffer_size = new_buf_size;
            printk(KERN_INFO "success modify read buffer size : %d \n",read_buffer_size);
            break;

        default:
            return -1;
    }
    return 0;
}

static int BufferedMem_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "BufferedMem_open!\n");
    return 0;
}

static int BufferedMem_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "BufferedMem_release!\n");
    return 0;
}

static struct file_operations BufferedMem_fops = {
    .owner = THIS_MODULE,
    .open = BufferedMem_open,
    .release = BufferedMem_release,
    .write = BufferedMem_write,
    .read = BufferedMem_read,
    .unlocked_ioctl = BufferedMem_ioctl,
};

static int BufferedMem_init(void)
{
    printk(KERN_INFO "BufferedMem_init\n");
    
    /* 내부 버퍼 할당 */
    if (kfifo_alloc(&fifo_buffer, buffer_size, GFP_KERNEL)){
        printk(KERN_ERR "error kfifo_alloc\n");
    }
    printk(KERN_INFO "kfifo fifo_buffer, size : %d\n",kfifo_size(&fifo_buffer));
    
    /* 디바이스 드라이버 등록 */
    register_chrdev(DEV_MAJOR_NUMBER, DEV_NAME, &BufferedMem_fops);
    printk(KERN_INFO "Device driver registered\n");
    
    return 0;
}

static void BufferedMem_exit(void)
{
    printk(KERN_INFO "BufferedMem_exit\n");

    /* 내부 버퍼 반납 */
    kfifo_free(&fifo_buffer);
    printk(KERN_INFO "free fifo_buffer\n");

    /* 디바이스 드라이버 제거 */
    unregister_chrdev(DEV_MAJOR_NUMBER, DEV_NAME);
    printk(KERN_INFO "Device driver unregistered\n");
}

module_init(BufferedMem_init);
module_exit(BufferedMem_exit);
MODULE_LICENSE("GPL");
