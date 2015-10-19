cat < /etc/resolv.conf

(cat) < /etc/resolv.conf

(ls | (wc))


(ls) | wc


ls | (wc)


#Error if sfoo.txt DNE, should work if it does
(cat) < sfoo.txt > sfoo2.txt


false && (echo hi1)


(false) && echo hi2


exec echo hi5