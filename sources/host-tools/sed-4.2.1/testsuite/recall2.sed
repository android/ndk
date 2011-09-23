# Starting from sed 4.1.3, regexes are compiled with REG_NOSUB
# if they are used in an address, so that the matcher does not
# have to obey leftmost-longest.  The tricky part is to recompile
# them if they are then used in a substitution.
/\(ab*\)\+/ s//>\1</g
