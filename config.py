major = 12
hotfix = 0
hotfix_str = chr(ord('a') + hotfix) if hotfix else ''
release = 'r{}{}'.format(major, hotfix_str)
