#
# Test the effect of input buffering on the shell's input
#
echo this is redir-test-3.sh

exec 0< redir-test-3.sub

echo after exec in redir-test-3.sh
