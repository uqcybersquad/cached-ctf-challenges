#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/random.h>
#include <linux/delay.h>

#define DEVICE_NAME "steam_engines"
#define CLASS_NAME  "steam_engines"

MODULE_DESCRIPTION("The new era of automated steam engine management for flying locomotives!");
MODULE_LICENSE("GPL");

#define MAX_ENGINES 0x100
#define MAX_COMPARTMENTS 0x200

#define NAME_SZ 0x28
#define DESC_SZ 0x70
#define LOG_SZ 0x100

#define ADD_ENGINE 0xc00010ff
#define ADD_COMPARTMENT 0x1337beef
#define DELETE_COMPARTMENT 0xdeadbeef
#define SHOW_ENGINE_LOG 0xcafebeef
#define UPDATE_ENGINE_LOG 0xbaadbeef

typedef int32_t id_t;

static long steam_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static long add_engine(char *name);
static long add_compartment(char *desc, id_t target_engine);
static long delete_compartment(id_t target_compartment);
static long show_engine_log(id_t target_compartment, char *logs);
static long update_engine_log(id_t target_compartment, char *logs);
static long find_empty_slot(uint64_t **arr, int max);
static long find_selection(id_t **arr, int max, int id);
static long automated_engine_shutdown(void);

typedef struct
{
    id_t id;
    uint8_t usage;
    char engine_name[NAME_SZ];
    char *logs;
}engine_t;

typedef struct
{
    id_t id;
    char compartment_desc[DESC_SZ];
    engine_t *engine;
}compartment_t;

typedef struct
{
    id_t id;
    char *name;
    char *desc;
    char *logs;
}req_t;

engine_t *engines[MAX_ENGINES];
compartment_t *compartments[MAX_COMPARTMENTS];
static struct miscdevice steam_dev;
static struct file_operations steam_fops = {.unlocked_ioctl = steam_ioctl};

static long find_empty_slot(uint64_t **arr, int max)
{
    int i;
    for (i = 0; i < max; i++)
    {
        if (!arr[i])
        {
            return i;
        }
    }
    return -1;
}

static long find_selection(id_t **arr, int max, int id)
{
    int i;
    for (i = 0; i < max; i++)
    {
        if (arr[i])
        {
            id_t chunk_id = *(arr[i]);
            if (chunk_id == id)
            {
                return i;
            }
        }
    }
    return -1;
}

static long automated_engine_shutdown(void)
{
    int i;
    long counter = 0;
    for (i = 0; i < MAX_ENGINES; i++)
    {
        if (engines[i] && !engines[i]->usage)
        {
            kfree(engines[i]->logs);
            engines[i]->logs = NULL;
            kfree(engines[i]);
            engines[i] = NULL;
            counter++;
        }
    }
    return counter;
}

static long steam_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long result;
    req_t req;
    if (copy_from_user(&req, (req_t *)arg, sizeof(req)))
    {
        return -1;
    }
    switch (cmd)
    {
        case ADD_ENGINE:
            result = add_engine(req.name);
            break;
        case ADD_COMPARTMENT:
            result = add_compartment(req.desc, req.id);
            break;
        case DELETE_COMPARTMENT:
            result = delete_compartment(req.id);
            break;
        case SHOW_ENGINE_LOG:
            result = show_engine_log(req.id, req.logs);
            break;
        case UPDATE_ENGINE_LOG:
            result = update_engine_log(req.id, req.logs);
            break;
        default:
            result = -1;
            break;
    }
    return result;
}

static long add_engine(char *name)
{
    int idx;
    automated_engine_shutdown();
    idx = find_empty_slot((uint64_t **)engines, MAX_ENGINES);
    if (idx < 0)
    {
        return -1;
    }
    engines[idx] = kzalloc(sizeof(engine_t), GFP_ATOMIC);
    if (!engines[idx])
    {
        return -1;
    }
    engines[idx]->logs = kzalloc(LOG_SZ, GFP_ATOMIC);
    if (!(engines[idx]->logs) || copy_from_user(engines[idx]->engine_name, name, NAME_SZ))
    {
        kfree(engines[idx]);
        engines[idx] = NULL;
        return -1;
    }
    get_random_bytes(&engines[idx]->id, sizeof(id_t));
    printk(KERN_INFO "Engine Added Successfully!\n");
    return (long)engines[idx]->id;
}

static long add_compartment(char *desc, id_t target_engine)
{
    int target_idx, alloc_idx;
    target_idx = find_selection((id_t **)engines, MAX_ENGINES, target_engine);
    if (target_idx < 0)
    {
        return -1;
    }
    if (engines[target_idx]->usage == 0xff)
    {
        return -1;
    }
    alloc_idx = find_empty_slot((uint64_t **)compartments, MAX_COMPARTMENTS);
    if (alloc_idx < 0)
    {
        return -1;
    }
    compartments[alloc_idx] = kzalloc(sizeof(compartment_t), GFP_ATOMIC);
    if (!compartments[alloc_idx])
    {
        return -1;
    }
    get_random_bytes(&compartments[alloc_idx]->id, sizeof(id_t));
    if (copy_from_user(compartments[alloc_idx]->compartment_desc, desc, DESC_SZ))
    {
        kfree(compartments[alloc_idx]);
        compartments[alloc_idx] = NULL;
        return -1;
    }
    compartments[alloc_idx]->engine = engines[target_idx];
    engines[target_idx]->usage++;
    automated_engine_shutdown();
    printk(KERN_INFO "New Compartment Connected to Engine Successfully!\n");
    return (long)compartments[alloc_idx]->id;
}

static long delete_compartment(id_t target_compartment)
{
    int idx;
    idx = find_selection((id_t **)compartments, MAX_COMPARTMENTS, target_compartment);
    if (idx < 0)
    {
        return -1;
    }
    compartments[idx]->engine->usage--;
    kfree(compartments[idx]);
    compartments[idx] = NULL;
    printk(KERN_INFO "Compartment Unlinked from Engine Successfully!\n");
    return 0;
}

static long show_engine_log(id_t target_compartment, char *log)
{
    int idx;
    idx = find_selection((id_t **)compartments, MAX_COMPARTMENTS, target_compartment);
    if (idx < 0)
    {
        return -1;
    }
    if (!copy_to_user(log, compartments[idx]->engine->logs, LOG_SZ))
    {
        printk(KERN_INFO "Maintenance Logs Read Successfully!\n");
        return 0;
    }
    return -1;
}

static long update_engine_log(id_t target_compartment, char *log)
{
    int idx;
    idx = find_selection((id_t **)compartments, MAX_COMPARTMENTS, target_compartment);
    if (idx < 0)
    {
        return -1;
    }
    if (!copy_from_user(compartments[idx]->engine->logs, log, LOG_SZ))
    {
        printk(KERN_INFO "Maintenance Logs Updated Successfully\n");
        return 0;
    }
    return -1;    
}

static int init_steam_driver(void)
{
    steam_dev.minor = MISC_DYNAMIC_MINOR;
    steam_dev.name = "steam";
    steam_dev.fops = &steam_fops;
    if (misc_register(&steam_dev))
    {
        return -1;
    }
    printk(KERN_INFO "Steam Driver Initialized\n");
    printk(KERN_INFO "Hope you enjoy our new steam engine management system!\n");
    return 0;
}

static void cleanup_steam_driver(void)
{
    misc_deregister(&steam_dev);
    printk(KERN_INFO "Shutting down steam system immediately!\n");
}

module_init(init_steam_driver);
module_exit(cleanup_steam_driver);