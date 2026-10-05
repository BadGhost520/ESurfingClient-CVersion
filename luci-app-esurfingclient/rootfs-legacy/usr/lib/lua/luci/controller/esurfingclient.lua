module("luci.controller.esurfingclient", package.seeall)

function index()
    entry({"admin", "services", "esurfingclient"}, template("esurfingclient"), "ESurfing 客户端", 10)
end