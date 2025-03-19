#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "process.h"
#include "filesys/filesys.h"
#include "lib/string.h"
#include "filesys/directory.h"

static void syscall_handler (struct intr_frame *);
int pipe(int *fd);
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);
}

// Verify the validity of a user-provided pointer.
bool is_valid_pointer(const void *vaddr)
{
  uint32_t *pd = thread_current()->pagedir;
  // Use the functions in ‘userprog/pagedir.c’ and in ‘threads/vaddr.h’ to verify the validity of the pointer.
  return vaddr != NULL&& is_user_vaddr(vaddr) && pagedir_get_page(pd, vaddr) != NULL;
}

bool is_valid_string(const char *str) {
    if (str == NULL || *str == '\0') 
        return false;
    
    const char *ptr = str;
    uint32_t *pd = thread_current()->pagedir;
    while (is_user_vaddr(ptr)) {
        if (pagedir_get_page(pd, ptr) == NULL)
            {return false;}
        if (*ptr == '\0') 
           { return true;}
        ptr++;

    }
    return false; // Either string was too long or invalid
}


static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // Make system call handler call system call using system call number
  // which is at the top of the stack.
  int syscall_num = *((int *)(f->esp)); //esp is pointer to where stack starts
  uint32_t arg0, arg1, arg2;

  // Check validation of the pointers in the parameter list.
  // Copy arguments on the user stack to the kernel.
  // Save return value of system call at eax register.
  switch (syscall_num)
  {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      if (!is_valid_pointer(f->esp + 4)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4));
      exit(arg0);
      f->eax = arg0;
      break;
    case SYS_EXEC:
      if (!is_valid_pointer(f->esp + 4)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4));
      f->eax = exec((const char *)arg0);
      break;
    case SYS_WAIT:
      if (!is_valid_pointer(f->esp + 4)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4));
      f->eax = wait((pid_t)arg0);
      break;
    case SYS_CREATE:
      if (!is_valid_pointer(f->esp + 4) || !is_valid_pointer(f->esp + 8)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4));
      arg1 = *((uint32_t *)(f->esp + 8));
      f->eax = create((const char *)arg0, (unsigned)arg1);
      break;
    case SYS_REMOVE:
      if (!is_valid_pointer(f->esp + 4)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4));
      f->eax = remove((const char *)arg0);
      break;
    case SYS_OPEN:
      if (!is_valid_pointer(f->esp + 4)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4));
      f->eax = open((const char *)arg0);
      break;
    case SYS_FILESIZE:
      if (!is_valid_pointer(f->esp + 4)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4));
      f->eax = filesize((int)arg0);
      break;
    case SYS_READ:
      if (!is_valid_pointer(f->esp + 4) || !is_valid_pointer(f->esp + 8) || !is_valid_pointer(f->esp + 12)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4));
      arg1 = *((uint32_t *)(f->esp + 8));
      arg2 = *((uint32_t *)(f->esp + 12));
      f->eax = read((int)arg0, (void *)arg1, (unsigned)arg2);
      break;
    case SYS_WRITE:
      if (!is_valid_pointer(f->esp + 4) || !is_valid_pointer(f->esp + 8) || !is_valid_pointer(f->esp + 12)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4)); //because they recieve them from a stack. So, we have to move and get them
      arg1 = *((uint32_t *)(f->esp + 8));
      arg2 = *((uint32_t *)(f->esp + 12));
      f->eax = write((int)arg0, (const void *)arg1, (unsigned)arg2);
      break;
    case SYS_SEEK:
      if (!is_valid_pointer(f->esp + 4) || !is_valid_pointer(f->esp + 8)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4));
      arg1 = *((uint32_t *)(f->esp + 8));
      seek((int)arg0, (unsigned)arg1);
      break;
    case SYS_TELL:
      if (!is_valid_pointer(f->esp + 4)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4));
      f->eax = tell((int)arg0);
      break;
    case SYS_CLOSE:
      if (!is_valid_pointer(f->esp + 4)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4));
      close((int)arg0);
      break;
    case SYS_PIPE:
      if (!is_valid_pointer(f->esp + 4)) exit(-1);
      arg0 = *((uint32_t *)(f->esp + 4));
      f->eax = pipe((int *)arg0);
      break;
    default:
      printf("Unknown system call number: %d\n", syscall_num);
      exit(-1);
  }
}

// Shutdown pintos
void halt(void)
{
  shutdown_power_off ();
}

// Terminates the current user program, returning status to the kernel. 
// If the process’s parent waits for it, this is the status that will be returned. Conventionally, a status of 0 indicates success and nonzero values indicate errors
void exit(int status)
{

  struct thread *cur = thread_current();
  printf("%s: exit(%d)\n", cur->name, status);
  cur->exit_status = status;
  // printf("TID %d exit status: %d\n", cur->tid, cur->exit_status);
  if(cur->parent != NULL)
   { 
    // printf("Sema up for wait_sema\n");
    sema_up(&cur->wait_sema);
       sema_down(&cur->exit_sema);
       }
 


  thread_exit();
}
  
// Create child process and execute program corresponds to cmd_line on it
pid_t exec (const char *cmd_line)
{
  // printf("exec syscall (%s)\n", cmd_line);
  // printf("returning tid from EXEC: %d\n", tid);
  return process_execute(cmd_line);
}

// Wait for termination of child process whose process id is pid
int wait (pid_t pid)
{
  // printf("wait syscall (%d)\n", pid);
  return process_wait(pid);
}

bool create(const char *file, unsigned initial_size)
{
  // Create file which have size of initial_size.
  // Use bool filesys_create(const char *name, off_t initial_size).
  // Return true if it is succeeded or false if it is not.
  if (file == NULL) exit(-1);
        //check if the pointer is valid and the string is valid for file name and the length of the file name is less than 14
      if (!is_valid_pointer((const void *)file) || !is_valid_string((const char *)file))
        exit(-1);
    if (strlen(file) > NAME_MAX)
        return false;
  
  lock_acquire(&file_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&file_lock);

  return success;
}
  

bool remove(const char *file)
{
  // Remove file whose name is file.
  // Use bool filesys_remove(const char *name).
  // Return true if it is succeeded or false if it is not.
  // File is removed regardless of whether it is open or closed.
  if (file == NULL) exit(-1);

  lock_acquire(&file_lock);
  bool success = filesys_remove(file);
  lock_release(&file_lock);
  return success;
}

int open(const char *file)
{
  // Open the file corresponds to path in “file”.
  // Return its fd.

  if (file == NULL) exit(-1);
  lock_acquire(&file_lock);
  struct file *f = filesys_open(file);
  lock_release(&file_lock);
  if (f == NULL) return -1;

  struct thread *cur = thread_current();
  int fd = cur->next_fd;
  cur->fdt[fd] = f;
  cur->next_fd++;
  return fd;
}
int filesize(int fd)
{
  // Return the size, in bytes, of the file open as fd.
  // Use off_t file_length(struct file *file).
  if (fd < 2 || fd >= thread_current()->next_fd || thread_current()->fdt[fd] == NULL) return -1;
  lock_acquire(&file_lock);
  int size = file_length(thread_current()->fdt[fd]);
  lock_release(&file_lock);
  return size;
}
int read(int fd, void *buffer, unsigned size)
{
  // Read size bytes from the file open as fd into buffer.
  // Return the number of bytes actually read (0 at end of file), or -1 if fails.
  // If fd is 0, it reads from keyboard using input_getc(), otherwise reads from file using file_read() function.
  // uint8_t input_getc(void)
  // off_t file_read(struct file *file, void *buffer, off_t size)
  if (buffer == NULL) exit(-1);
  if (!is_valid_pointer((const void *)buffer))
        exit(-1);


  if (fd == 0)
  {
    lock_acquire(&file_lock);
    uint8_t *buf = (uint8_t *)buffer;
    unsigned i;
    for (i = 0; i < size; i++)
    {
      buf[i] = input_getc();
    }
    lock_release(&file_lock);
    return size;
  }
  else
  {
    if (fd < 2 || fd >= thread_current()->next_fd || thread_current()->fdt[fd] == NULL) return -1;
    lock_acquire(&file_lock);
    int bytes_read = file_read(thread_current()->fdt[fd], buffer, size);
    lock_release(&file_lock);
    return bytes_read;
  }
}

int write(int fd, const void *buffer, unsigned size)
{
  // Writes size bytes from buffer to the open file fd.
  // Returns the number of bytes actually written.
  // If fd is 1, it writes to the console using putbuf(), otherwise write to the file using file_write() function.
  // void putbuf(const char *buffer, size_t n)
  // off_t file_write(struct file *file, const void *buffer, off_t size)
  if (buffer == NULL) exit(-1);
  if (fd == 1)
  {
    lock_acquire(&file_lock);
    putbuf(buffer, size);
    lock_release(&file_lock);
    return size;
  }
  else
  {
    if (fd < 2 || fd >= thread_current()->next_fd || thread_current()->fdt[fd] == NULL) return -1;
    lock_acquire(&file_lock);
    int bytes_written = file_write(thread_current()->fdt[fd], buffer, size);
    lock_release(&file_lock);
    return bytes_written;
  }
}
void seek(int fd, unsigned position)
{
  // Changes the next byte to be read or written in open file fd to position.
  // Use void file_seek(struct file *file, off_t new_pos).
  if (fd < 2 || fd >= thread_current()->next_fd || thread_current()->fdt[fd] == NULL) return;
  lock_acquire(&file_lock);
  file_seek(thread_current()->fdt[fd], position);
  lock_release(&file_lock);
  return;
}
unsigned tell(int fd)
{
  // Return the position of the next byte to be read or written in open file fd.
  // Use off_t file_tell(struct file *file).
  if (fd < 2 || fd >= thread_current()->next_fd || thread_current()->fdt[fd] == NULL) return -1;
  lock_acquire(&file_lock);
  int position = file_tell(thread_current()->fdt[fd]);
  lock_release(&file_lock);
  return (unsigned)position;
}
void close(int fd)
{
  // Close file descriptor fd.
  // Use void file_close(struct file *file).
  if (fd < 2 || fd >= thread_current()->next_fd || thread_current()->fdt[fd] == NULL) return;
  lock_acquire(&file_lock);
  file_close(thread_current()->fdt[fd]);
  thread_current()->fdt[fd] = NULL;
  lock_release(&file_lock);
  return;
}




int pipe(int *fd)
{
  return -1;
}


