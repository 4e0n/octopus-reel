#!/bin/bash

find_lineno() {
 local TMP=$(echo $(cat $1 | grep -n "$2" | cut -d: -f1))
 LN=$(echo $TMP | awk '{print $1}') # pick first occurence
}

del_lineno() {
 mv $1 /tmp/3141516
 sed "$2d" /tmp/3141516 > $1
}

del_lineno_interval() {
 mv $1 /tmp/3141516
 sed "$2,$3d" /tmp/3141516 > $1
}

insert_line() {
 mv $1 /tmp/3141516
 awk -v "n=$(($2+1))" -v "s=$3" '(NR==n) { print s } 1' /tmp/3141516 > $1
}

comment_line_cont_text() {
 mv $1 /tmp/3141516
 sed -e "/$2/s/^/$3/g" /tmp/3141516 > $1 
}

# POST-HOC STATIC IP SETTINGS
# IP parameters are set to comply to settings from DHCP server. After installation all is converted static C-type private IP settings with predetermined IPs for different octopus-reel hosts.
HN="leviathan"
DN="octopus0"
GW=$(route -n | grep UG | awk '{print $2}')
IP=$(echo $GW | sed 's/[^.]*$/10/')
NW=$(echo $GW | sed 's/[^.]*$/0/')
NM=$(echo 255.255.255.0)
IP_FOR=$(echo "$(echo $IP | cut -d. -f1).$(echo $IP | cut -d. -f2).$(echo $IP | cut -d. -f3)") # e.g. 10.0.17.x
IP_REV=$(echo "$(echo $IP | cut -d. -f3).$(echo $IP | cut -d. -f2).$(echo $IP | cut -d. -f1)") # e.g. x.17.0.10 , without (pre/ap)pended dot and x.

#-----------

# Set static IP with DNS of our own to be activated during next boot
mv /etc/network/interfaces /etc/network/interfaces.backup
sed -e 's/allow-hotplug/auto/g' -e 's/inet dhcp/inet static/' /etc/network/interfaces.backup >/etc/network/interfaces
echo -e " address $IP" >> /etc/network/interfaces
echo -e " netmask $NM" >> /etc/network/interfaces
echo -e " gateway $GW" >> /etc/network/interfaces
echo -e " dns-nameservers $IP" >> /etc/network/interfaces
echo -e " dns-search $DN" >> /etc/network/interfaces

# Set sharing of /home directory via NFS-server
echo -e "/home $NW/24(rw,no_root_squash)\n" >> /etc/exports
find_lineno /etc/idmapd.conf localdomain
insert_line /etc/idmapd.conf $LN "Domain = $DN"

# Set Octopus-ReEL DNS of networked boxes
cp /etc/bind/named.conf /etc/bind/named.conf.backup
cp /etc/bind/named.conf.options /etc/bind/named.conf.options.backup
cp /etc/bind/named.conf.local /etc/bind/named.conf.local.backup
cp /etc/bind/named.conf.internal-zones /etc/bind/named.conf.internal-zones.backup
comment_line_cont_text /etc/bind/named.conf "default-zones" "\/\/"
echo -e "include \"/etc/bind/named.conf.internal-zones\";\n//include \"/etc/bind/named.conf.external-zones\";" >> /etc/bind/named.conf.local
echo -e "view \"internal\" {\n\tmatch-clients {\n\t\tlocalhost;\n\t\t$NW/24;\n\t};\n" >> /etc/bind/named.conf.internal-zones
echo -e "\tzone \"octopus0\" {\n\t\ttype master;\n\t\tfile \"/etc/bind/octopus0.lan\";\n\t\tallow-update { none; };\n\t};\n" >> /etc/bind/named.conf.internal-zones
echo -e "\tzone \"$IP_REV.in-addr.arpa\" {\n\t\ttype master;\n\t\tfile \"/etc/bind/$IP_REV.db\";\n\t\tallow-update { none; };\n\t};\ninclude \"/etc/bind/named.conf.default-zones\";\n};" >> /etc/bind/named.conf.internal-zones

find_lineno /etc/bind/named.conf.options '// forwarders {'
LN=$(($LN+2))
insert_line /etc/bind/named.conf.options $LN "\tallow-recursion { localhost; ${NW}/24; };"
insert_line /etc/bind/named.conf.options $LN "\tallow-transfer { localhost; ${NW}/24; };"
insert_line /etc/bind/named.conf.options $LN "\tallow-query { localhost; ${NW}/24; };"

comment_line_cont_text /etc/bind/named.conf.options "listen-on-v6" "\/\/"
find_lineno /etc/bind/named.conf.options 'listen-on-v6'
insert_line /etc/bind/named.conf.options $LN "\tlisten-on-v6 { none; };"

touch /etc/bind/octopus0.lan
echo -e "\$TTL 86400\n@\tIN\tSOA\t\tleviathan.octopus0. root.octopus0. (" >> /etc/bind/octopus0.lan
echo -e "\t\t$(date +%Y%m%d)01\t;Serial" >> /etc/bind/octopus0.lan
echo -e "\t\t3600\t\t;Refresh" >> /etc/bind/octopus0.lan
echo -e "\t\t1800\t\t;Retry" >> /etc/bind/octopus0.lan
echo -e "\t\t604800\t\t;Expire" >> /etc/bind/octopus0.lan
echo -e "\t\t86400\t\t;Minimum TTL\n)" >> /etc/bind/octopus0.lan
echo -e "\t\tIN NS\t\tleviathan.octopus0." >> /etc/bind/octopus0.lan
echo -e "\t\tIN A\t\t${IP}" >> /etc/bind/octopus0.lan
echo -e "\t\tIN MX 10\tleviathan.octopus0.\n" >> /etc/bind/octopus0.lan
echo -e ";---------------------------------------------------------\n" >> /etc/bind/octopus0.lan
echo -e "leviathan\tIN A\t\t${IP}" >> /etc/bind/octopus0.lan
echo -e "dns\t\tIN CNAME\tleviathan.octopus0.\n" >> /etc/bind/octopus0.lan

echo -e "router0\t\tIN A\t\t${GW}" >> /etc/bind/octopus0.lan

echo -e "cmaster\t\tIN A\t\t${IP_FOR}.11\n" >> /etc/bind/octopus0.lan
echo -e "fury\t\tIN A\t\t${IP_FOR}.19\n" >> /etc/bind/octopus0.lan
echo -e "stm0\t\tIN A\t\t${IP_FOR}.20\n" >> /etc/bind/octopus0.lan
echo -e "acq0\t\tIN A\t\t${IP_FOR}.30\n" >> /etc/bind/octopus0.lan

echo -e "cn0\t\tIN A\t\t${IP_FOR}.100" >> /etc/bind/octopus0.lan
echo -e "cn1\t\tIN A\t\t${IP_FOR}.101" >> /etc/bind/octopus0.lan
echo -e "cn2\t\tIN A\t\t${IP_FOR}.102" >> /etc/bind/octopus0.lan
echo -e "cn3\t\tIN A\t\t${IP_FOR}.103" >> /etc/bind/octopus0.lan
echo -e "cn4\t\tIN A\t\t${IP_FOR}.104" >> /etc/bind/octopus0.lan
echo -e "cn5\t\tIN A\t\t${IP_FOR}.105" >> /etc/bind/octopus0.lan
echo -e "cn6\t\tIN A\t\t${IP_FOR}.106" >> /etc/bind/octopus0.lan
echo -e "cn7\t\tIN A\t\t${IP_FOR}.107\n" >> /etc/bind/octopus0.lan

echo -e "gn0\t\tIN A\t\t${IP_FOR}.150" >> /etc/bind/octopus0.lan
echo -e "gn1\t\tIN A\t\t${IP_FOR}.151" >> /etc/bind/octopus0.lan
echo -e "gn2\t\tIN A\t\t${IP_FOR}.152" >> /etc/bind/octopus0.lan
echo -e "gn3\t\tIN A\t\t${IP_FOR}.153" >> /etc/bind/octopus0.lan
echo -e "gn4\t\tIN A\t\t${IP_FOR}.154" >> /etc/bind/octopus0.lan
echo -e "gn5\t\tIN A\t\t${IP_FOR}.155" >> /etc/bind/octopus0.lan
echo -e "gn6\t\tIN A\t\t${IP_FOR}.156" >> /etc/bind/octopus0.lan
echo -e "gn7\t\tIN A\t\t${IP_FOR}.157" >> /etc/bind/octopus0.lan

REV_FN="/etc/bind/$IP_REV.db"
echo -e "\$TTL 86400\n@\tIN\tSOA\t\tleviathan.octopus0. root.octopus0. (" >> $REV_FN
echo -e "\t\t$(date +%Y%m%d)01\t;Serial" >> $REV_FN
echo -e "\t\t3600\t\t;Refresh" >> $REV_FN
echo -e "\t\t1800\t\t;Retry" >> $REV_FN
echo -e "\t\t604800\t\t;Expire" >> $REV_FN
echo -e "\t\t86400\t\t;Minimum TTL\n)" >> $REV_FN
echo -e "\t\tIN NS\t\tleviathan.octopus0." >> $REV_FN
echo -e "\t\tIN PTR\t\toctopus0." >> $REV_FN
echo -e "\t\tIN A\t\t255.255.255.0\n" >> $REV_FN
echo -e ";---------------------------------------------------------\n" >> $REV_FN
echo -e "10\t\tIN PTR\t\tleviathan.octopus0.\n" >> $REV_FN
echo -e "1\t\tIN PTR\t\trouter0.octopus0.\n" >> $REV_FN
echo -e "19\t\tIN PTR\t\tfury.octopus0.\n" >> $REV_FN
echo -e "20\t\tIN PTR\t\tstm0.octopus0.\n" >> $REV_FN
echo -e "30\t\tIN PTR\t\tacq0.octopus0.\n" >> $REV_FN

echo -e "100\t\tIN PTR\t\tcn0.octopus0." >> $REV_FN
echo -e "101\t\tIN PTR\t\tcn1.octopus0." >> $REV_FN
echo -e "102\t\tIN PTR\t\tcn2.octopus0." >> $REV_FN
echo -e "103\t\tIN PTR\t\tcn3.octopus0." >> $REV_FN
echo -e "104\t\tIN PTR\t\tcn4.octopus0." >> $REV_FN
echo -e "105\t\tIN PTR\t\tcn5.octopus0." >> $REV_FN
echo -e "106\t\tIN PTR\t\tcn6.octopus0." >> $REV_FN
echo -e "107\t\tIN PTR\t\tcn7.octopus0." >> $REV_FN

echo -e "150\t\tIN PTR\t\tgn0.octopus0." >> $REV_FN
echo -e "151\t\tIN PTR\t\tgn1.octopus0." >> $REV_FN
echo -e "152\t\tIN PTR\t\tgn2.octopus0." >> $REV_FN
echo -e "153\t\tIN PTR\t\tgn3.octopus0." >> $REV_FN
echo -e "154\t\tIN PTR\t\tgn4.octopus0." >> $REV_FN
echo -e "155\t\tIN PTR\t\tgn5.octopus0." >> $REV_FN
echo -e "156\t\tIN PTR\t\tgn6.octopus0." >> $REV_FN
echo -e "157\t\tIN PTR\t\tgn7.octopus0." >> $REV_FN

# Set NIS master server for Octopus-ReEL user authentication
comment_line_cont_text /etc/default/nis "NISSERVER=false" "#"
find_lineno /etc/default/nis 'NISSERVER=false'
insert_line /etc/default/nis $LN "NISSERVER=master"

comment_line_cont_text /etc/ypserv.securenets "0.0.0.0" "#"
echo -e "255.255.255.0\t$NW" >> /etc/ypserv.securenets

comment_line_cont_text /var/yp/Makefile "MERGE_PASSWD=false" "#"
find_lineno /var/yp/Makefile 'MERGE_PASSWD=false'
insert_line /var/yp/Makefile $LN "MERGE_PASSWD=true"
comment_line_cont_text /var/yp/Makefile "MERGE_GROUP=false" "#"
find_lineno /var/yp/Makefile 'MERGE_GROUP=false'
insert_line /var/yp/Makefile $LN "MERGE_GROUP=true"

comment_line_cont_text /var/yp/Makefile "MINUID=1000" "#"
find_lineno /var/yp/Makefile 'MINUID=1000'
insert_line /var/yp/Makefile $LN "MINUID=10000"
comment_line_cont_text /var/yp/Makefile "MINGID=1000" "#"
find_lineno /var/yp/Makefile 'MINGID=1000'
insert_line /var/yp/Makefile $LN "MINGID=10000"

# Doctor ypinit for non-interactive usage for a single time
cp /usr/lib/yp/ypinit /opt/ypinit.0
find_lineno /usr/lib/yp/ypinit 'read hostlist_ok'
del_lineno /usr/lib/yp/ypinit $LN
find_lineno /usr/lib/yp/ypinit 'while read h'
del_lineno_interval /usr/lib/yp/ypinit $LN $(($LN+4))
chmod 700 /usr/lib/yp/ypinit

# Post-install script for the first boot after installation
# attached just before nis init.d script to delete itself afterwards
touch /opt/octopus_nis_init.sh
chmod 700 /opt/octopus_nis_init.sh
echo -e "#!/bin/bash\n" >> /opt/octopus_nis_init.sh
echo -e "/usr/lib/yp/ypinit -m" >> /opt/octopus_nis_init.sh
echo -e "mv /opt/ypinit.0 /usr/lib/yp/ypinit" >> /opt/octopus_nis_init.sh # restore original
echo -e "cd /var/yp\nmake" >> /opt/octopus_nis_init.sh

find_lineno /etc/init.d/nis '### END INIT INFO'
insert_line /etc/init.d/nis $LN "\nif [ -e /opt/octopus_nis_init.sh ]\nthen\n /opt/octopus_nis_init.sh\n logger \"Octopus-ReEL: NIS successfully initialized.\"\n rm -f /opt/octopus_nis_init.sh\nfi\n"
chmod 755 /etc/init.d/nis

#------ EOF
