#!/bin/sh -
#
# This program generates entry.c from syscall.dat

cat << \Trailer
/* Switched to V7 system call layout... Chad - 1/5/96
 */
#asm

!	The call table - autogenerated from syscall.dat

	.data
sys_call_table:
Trailer

tr '[A-Z]' '[a-z]' < syscall.dat | \
awk '/^#/{next;}
/^[	 ]$/{next;}
{
   callno = 0+$2;
   if( $4 != "-" )
      assigned_to[callno] = $1;

   if( $4 == "=" )
      depends_on[callno] = toupper($5);

   if( $3 == "x" || $3 == "" ) next;
   else if( $4 == "@" || $4 == "-" ) next;

   # Not implemented yet
   if( substr($2, 1, 1) != "+" ) next;

   if( maxno < callno ) maxno = callno;

   str = "\t.word _sys_" $1;
   line[callno] = sprintf("%-25s ! %3d", str, callno);
}
END{
   for(maxstd=maxno; depends_on[maxstd]!=""; maxstd--)
   {
      X = X;
   }
   for(callno=0; callno<=maxno; callno++)
   {
#      if( assigned_to[callno] == "fork" )
#        gsub("_sys_fork", "_do_fork ", line[callno]);

      if( assigned_to[callno] == "insmod" )
         gsub("_sys_insmod", "_module_init", line[callno]);

      if( depends_on[callno] != "" )
         printf "#ifdef %s\n", depends_on[callno]

      if( callno in line )
         print line[callno];
      else
      {
         if( assigned_to[callno] == "" )
            assigned_to[callno] = "unassigned";
#        if( assigned_to[callno] == "vfork" )
# {
#           str = "\t.word _do_fork";
# }
# else
            str = "\t.word _no_syscall";
         printf "%-25s ! %3d - %s\n", str, callno, assigned_to[callno];
      }

      if( depends_on[callno] != "" )
      {
         if( callno <= maxstd )
         {
            str = "\t.word _no_syscall";
            printf "#else\n%-25s ! %3d - %s\n", str, callno, assigned_to[callno]
         }

         printf "#endif\n"
      }
   }
}
'

cat <<\Trailer

sys_call_table_end:

!	Despatch a syscall (called from syscall_int)
!	Entry: ax=function code, stack contains parameters

	.text
	.globl _syscall

_syscall:
	cmp  ax,#((sys_call_table_end - sys_call_table)/2)
	ja   _nsyscall
	! look up address and jump to function
	mov  bx,ax
	shl  bx,#1		! multiply by 2
	add  bx,#sys_call_table
	j    [bx]

!	All unimplemented calls
	
_nsyscall:
	br  _no_syscall

#endasm
Trailer
