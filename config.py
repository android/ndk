major = 11
hotfix = 1
hotfix_str = chr(ord('a') + hotfix) if hotfix else ''
release = 'r{}{}'.format(major, hotfix_str)
