Not much consistency... for example:
Trade updates have orderType in {l,m} and side in {b,s}
ownTrades have ordertype in {limit, market} and type in {buy, sell}

Book update is very strange having 2 data items pushing
channelName + pair further down the array

Book has updateType -- republish means a price level reappears in
the view
