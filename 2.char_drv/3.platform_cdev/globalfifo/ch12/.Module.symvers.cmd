cmd_/home/administrator/Projects/develop/training/kernel/drivers/globalfifo/ch12/Module.symvers := sed 's/ko$$/o/' /home/administrator/Projects/develop/training/kernel/drivers/globalfifo/ch12/modules.order | scripts/mod/modpost  -a   -o /home/administrator/Projects/develop/training/kernel/drivers/globalfifo/ch12/Module.symvers -e -i Module.symvers   -T -