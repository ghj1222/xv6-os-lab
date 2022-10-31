// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock[NBUFBUCKET];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUFBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b, *h;
  char buf_lockname[16];
  for (int i = 0; i < NBUFBUCKET; i++) {
    snprintf(buf_lockname, 16, "bcache_%d", i);
    initlock(&bcache.lock[i], buf_lockname);
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  for (b = bcache.buf; b < bcache.buf+NBUF; b++) {
    initsleeplock(&b->lock, "buffer");
    h = &bcache.head[(b-bcache.buf)%NBUFBUCKET];
    b->next = h->next; b->prev = h;
    h->next->prev = b; h->next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b, *h;
  int id = (blockno*NDEV+dev-1) % NBUFBUCKET;
  acquire(&bcache.lock[id]);
  h = &bcache.head[id];
  // Is the block already cached?
  for (b = h->next; b != h; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  //printf("bget (%d,%d)=%d failed\n", dev, blockno, id);
  for (b = h->prev; b != h; b = b->prev) {
    if (b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[id]);
  for (int nid = (id+1)%NBUFBUCKET; nid != id; nid = (nid+1)%NBUFBUCKET) {
    acquire(&bcache.lock[nid]);
    h = &bcache.head[nid];
    for (b = h->prev; b != h; b = b->prev) {
      if (b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->prev->next=b->next;
        b->next->prev=b->prev;
        release(&bcache.lock[nid]);
        acquire(&bcache.lock[id]);
        h = &bcache.head[id];
        b->next=h->next;b->prev=h;
        h->next->prev=b;h->next=b;
        release(&bcache.lock[id]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[nid]);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  int id = (b->blockno*NDEV+b->dev-1)%NBUFBUCKET;
  releasesleep(&b->lock);
  
  acquire(&bcache.lock[id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    struct buf *h = &bcache.head[id];
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = h->next; b->prev = h;
    h->next->prev = b; h->next = b;
  }
  
  release(&bcache.lock[id]);
}

void
bpin(struct buf *b) {
  int id = (b->blockno * NDEV + b->dev-1)%NBUFBUCKET;
  acquire(&bcache.lock[id]);
  b->refcnt++;
  release(&bcache.lock[id]);
}

void
bunpin(struct buf *b) {
  int id = (b->blockno * NDEV + b->dev-1)%NBUFBUCKET;
  acquire(&bcache.lock[id]);
  b->refcnt--;
  release(&bcache.lock[id]);
}


