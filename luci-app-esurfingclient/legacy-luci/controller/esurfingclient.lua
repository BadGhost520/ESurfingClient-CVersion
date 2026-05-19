module("luci.controller.esurfingclient", package.seeall)

function index()
    entry({"admin", "services", "esurfingclient"}, template("esurfingclient/config"), _("ESurfing 客户端"), 90)
end