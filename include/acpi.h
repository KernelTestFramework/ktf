#ifndef KTF_ACPI_H
#define KTF_ACPI_H

#include <ktf.h>
#include <lib.h>
#include <processor.h>
#include <mm/pmm.h>

#define RSDP_SIGNATURE (('R') | ('S' << 8) | ('D' << 16) | ('P' << 24))
#define RSDT_SIGNATURE (('R') | ('S' << 8) | ('D' << 16) | ('T' << 24))
#define XSDT_SIGNATURE (('X') | ('S' << 8) | ('D' << 16) | ('T' << 24))

#define MADT_SIGNATURE (('A') | ('P' << 8) | ('I' << 16) | ('C' << 24))

struct acpi_table_hdr {          /* ACPI common table header */
    uint32_t signature;          /* ACPI signature (4 ASCII characters) */
    uint32_t length;             /* Length of table, in bytes, including header */
    uint8_t  rev;                /* ACPI Specification minor version # */
    uint8_t  checksum;           /* To make sum of entire table == 0 */
    uint8_t  oem_id[6];          /* OEM identification */
    uint8_t  oem_table_id[8];    /* OEM table identification */
    uint32_t oem_rev;            /* OEM revision number */
    uint8_t  asl_compiler_id[4]; /* ASL compiler vendor ID */
    uint32_t asl_compiler_rev;   /* ASL compiler revision number */
} __packed;
typedef struct acpi_table_hdr acpi_table_hdr_t;

/* Root System Descriptor Pointer */
struct rsdp_rev1 {
    uint64_t signature;    /* ACPI signature, contains "RSD PTR " */
    uint8_t  checksum;     /* To make sum of struct == 0 */
    uint8_t  oem_id[6];    /* OEM identification */
    uint8_t  rev;          /* Must be 0 for 1.0, 2 for 2.0 */
    uint32_t rsdt_paddr;   /* 32-bit physical address of RSDT */
} __packed;
typedef struct rsdp_rev1 rsdp_rev1_t;

struct rsdp_rev2 {
    rsdp_rev1_t rev1;
    uint32_t length;       /* XSDT Length in bytes including hdr */
    uint64_t xsdt_paddr;   /* 64-bit physical address of XSDT */
    uint8_t  ext_checksum; /* Checksum of entire table */
    uint8_t  rsvd[3];      /* Reserved field must be 0 */
} __packed;
typedef struct rsdp_rev2 rsdp_rev2_t;

struct rsdt {
    acpi_table_hdr_t header;
    uint32_t entry[0];
} __packed;
typedef struct rsdt rsdt_t;

struct xsdt {
    acpi_table_hdr_t header;
    struct {
    uint32_t low;
    uint32_t high;
    } entry[0];
} __packed;
typedef struct xsdt xsdt_t;

struct acpi_table {
    acpi_table_hdr_t header;
    char data[0];
} __packed;
typedef struct acpi_table acpi_table_t;

struct acpi_fadt_rev1 {
    acpi_table_hdr_t header;
    uint32_t firmware_ctrl; /* Physical address of FACS */
    uint32_t dsdt;          /* Physical address of DSDT */
    uint8_t  model;         /* System Interrupt Model */
    uint8_t  rsvd1;         /* Reserved */
    uint16_t sci_int;       /* System vector of SCI interrupt */
    uint32_t smi_cmd;       /* Port address of SMI command port */
    uint8_t  acpi_enable;   /* Value to write to smi_cmd to enable ACPI */
    uint8_t  acpi_disable;  /* Value to write to smi_cmd to disable ACPI */
    uint8_t  S4bios_req;    /* Value to write to SMI CMD to enter S4BIOS state */
    uint8_t  rsvd2;         /* Reserved - must be zero */
    uint32_t pm1a_evt_blk;  /* Port address of Power Mgt 1a acpi_event Reg Blk */
    uint32_t pm1b_evt_blk;  /* Port address of Power Mgt 1b acpi_event Reg Blk */
    uint32_t pm1a_ctrl_blk; /* Port address of Power Mgt 1a Control Reg Blk */
    uint32_t pm1b_ctrl_blk; /* Port address of Power Mgt 1b Control Reg Blk */
    uint32_t pm2_ctrl_blk;  /* Port address of Power Mgt 2 Control Reg Blk */
    uint32_t pm_tmr_blk;    /* Port address of Power Mgt Timer Ctrl Reg Blk */
    uint32_t gpe0_blk;      /* Port addr of General Purpose acpi_event 0 Reg Blk */
    uint32_t gpe1_blk;      /* Port addr of General Purpose acpi_event 1 Reg Blk */
    uint8_t  pm1_evt_len;   /* Byte length of ports at pm1_x_evt_blk */
    uint8_t  pm1_ctrl_len;  /* Byte length of ports at pm1_x_cnt_blk */
    uint8_t  pm2_ctrl_len;  /* Byte Length of ports at pm2_cnt_blk */
    uint8_t  pm_tmr_len;    /* Byte Length of ports at pm_tm_blk */
    uint8_t  gpe0_blk_len;  /* Byte Length of ports at gpe0_blk */
    uint8_t  gpe1_blk_len;  /* Byte Length of ports at gpe1_blk */
    uint8_t  gpe1_base;     /* Offset in gpe model where gpe1 events start */
    uint8_t  rsvd3;         /* Reserved */
    uint16_t plvl2_lat;     /* Worst case HW latency to enter/exit C2 state */
    uint16_t plvl3_lat;     /* Worst case HW latency to enter/exit C3 state */
    uint16_t flush_size;    /* Size of area read to flush caches */
    uint16_t flush_stride;  /* Stride used in flushing caches */
    uint8_t  duty_offset;   /* Bit location of duty cycle field in p_cnt reg */
    uint8_t  duty_width;    /* Bit width of duty cycle field in p_cnt reg */
    uint8_t  day_alrm;      /* Index to day-of-month alarm in RTC CMOS RAM */
    uint8_t  mon_alrm;      /* Index to month-of-year alarm in RTC CMOS RAM */
    uint8_t  century;       /* Index to century in RTC CMOS RAM */
    uint8_t  rsvd4[3];      /* Reserved */
} __packed;
typedef struct acpi_fadt_rev1 acpi_fadt_rev1_t;

struct acpi_gas { /* Generic Address Structure */
  uint8_t address_space;
  uint8_t bit_width;
  uint8_t bit_offset;
  uint8_t access_size;
  uint64_t address;
};
typedef struct acpi_gas acpi_gas_t;

struct acpi_fadt_rev2 {
    acpi_fadt_rev1_t rev1;
    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;
    acpi_gas_t x_pm1a_evt_blk;
    acpi_gas_t x_pm1b_evt_blk;
    acpi_gas_t x_pm1a_ctrl_blk;
    acpi_gas_t x_pm1b_ctrl_blk;
    acpi_gas_t x_pm2_ctrl_blk;
    acpi_gas_t x_pm_tmr_blk;
    acpi_gas_t x_gpe0_blk;
    acpi_gas_t x_gpe1_blk;
} __packed;
typedef struct acpi_fadt_rev2 acpi_fadt_rev2_t;

struct acpi_madt_entry {
    uint8_t type;
    uint8_t len;
    char data[0];
} __packed;
typedef struct acpi_madt_entry acpi_madt_entry_t;

enum acpi_madt_type {
    ACPI_MADT_TYPE_LAPIC = 0,
    ACPI_MADT_TYPE_IOAPIC = 1,
    ACPI_MADT_TYPE_IRQ_SRC= 2,
    ACPI_MADT_TYPE_NMI= 4,
    ACPI_MADT_TYPE_LAPIC_ADDR = 5,
};
typedef enum acpi_madt_type acpi_madt_type_t;

struct acpi_madt_processor {
    uint8_t apic_proc_id;
    uint8_t apic_id;
    uint32_t flags;
} __packed;
typedef struct acpi_madt_processor acpi_madt_processor_t;

struct acpi_madt {
    acpi_table_hdr_t header;
    uint32_t lapic_addr;
    uint32_t flags;
    acpi_madt_entry_t entry[0];
} __packed;
typedef struct acpi_madt acpi_madt_t;

/* External Declarations */

extern acpi_table_t *acpi_find_table(uint32_t signature);

extern unsigned acpi_get_nr_cpus(void);
extern void init_acpi(void);

#endif /* KTF_ACPI_H */
