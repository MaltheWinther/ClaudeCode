Øvelse 7 26/2-26, MA
Application Layer, Transport Layer, UDP/IP_Socket_Programming
Udstyr:
1 virtuel Ubuntu Linux-maskine og 1 RPi5
UDP-Client/Server:
1. Skriv en iterativ UDP-server der afvikles på en RPi 5 med support for en client ad gangen, som kan 
modtage en forespørgsel fra en client. Serveren skal bruge port 9000
- Serveren skal startes med: ./measurement_server
Hvis serveren modtager et ”U” eller ”u” skal informationen i filen /proc/uptime returneres til 
client. Denne fil indeholder aktuel information om den samlede tid serveren har været 
kørende siden opstart.
- Hvis serveren modtager et ”L” eller ”l” skal informationen i filen /proc/loadavg returneres til 
client. Denne fil indeholder aktuel information om serverens aktuelle CPU-load.
- Hvis serveren modtager andet end beskrevet ovenover skal den returnere en fejlbesked til 
client.
2. Skriv en UDP-client kørende i en virtuel maskine, som kan sende en kommando i form af et bogstav 
”U”, ”u”, ”L”, ”l” som indtastes af operatøren til UDP-serveren. Når svaret fra UDP-serveren 
(beskrevet i punkt 1) modtages, udskrives dette svar til UDP-client’s bruger.
- Klienten startes med: ./get_measurement <ip-server> <cmd) f.eks:
./get_measurement 172.16.15.2 u
3. Programmeringssprog: C/C+



