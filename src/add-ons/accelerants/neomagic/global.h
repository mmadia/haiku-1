extern int fd;
extern shared_info *si;
extern area_id shared_info_area;
extern area_id regs_area, regs2_area;
extern vuint32 *regs, *regs2;
extern display_mode *my_mode_list;
extern area_id my_mode_list_area;
extern int accelerantIsClone;

extern mn_get_set_pci mn_pci_access;
extern mn_in_out_isa mn_isa_access;
extern mn_bes_data mn_bes_access;
