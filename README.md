# BypassFP
Aplicação de console que burla a proteção do executável utilizado em Ant-Hack FireProtect (Utilizando APIs nativas do Windows - Usermode)

Funções nativas importadas e utilizadas no processo:
__NtOpenProcess
NtResumeProcess (Utilizado para testes)
NtSuspendProcess
NtOpenThread
NtQueryInformationThread__

Funcionamento resumido:
1. O __ProcessId__ do processo é obtido após filtragem da listagem realizada por __CreateToolhelp32Snapshot__ (__TH32_SNAPPROCESS__) verificando cada processo com o nome especificado.
2. Obtem-se um identificador com direitos de acesso limitado (__PROCESS_SUSPEND_RESUME__) por __NtOpenProcess__.
3. Após a verificação de um identificador válido o processo é suspendido com __NtSuspendProcess__.
4. Todas threads do processo são verificadas com listagem __CreateToolhelp32Snapshot__ (__TH32CS_SNAPTHREAD__) e obtido acesso total (__THREAD_ALL_ACCESS__) via __NtOpenThread__ retornando um identificador válido.
  4.1. O identificador é utilizado para obter uma informação específica via __NtQueryInformationThread__, obtendo o endereço inicial da thread do processo (__ThreadQuerySetWin32StartAddress__).
  4.2. Todos módulos do processo são verificados com listagem __CreateToolhelp32Snapshot__ (__TH32CS_SNAPMODULE__) e calculado se a thread identificada está dentro do bloco de memória do módulo analisado (condição: __baseAddressThread >= baseAddressModule && baseAddressThread <= baseAddressModule + baseAddressSize__) se sim, retorna-se o caminho do módulo no sistema relacionado a thread.
  4.3. Verifica-se o modulo da thread é __Main.exe__, se sim, a thread é resumida novamente via __ResumeThread__.
 
 
  _Observações:
 1.Algumas APIs nativas não são documentadas pela Microsoft (NtResumeProcess e NtSuspendProcess).
 2.Divulgação desse projeto é de cunho educativo.
 3.Não ajudo a burlar jogos on-line, evite contato.
 _
 
 Bons estudos!
 
