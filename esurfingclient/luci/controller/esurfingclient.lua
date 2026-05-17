module("luci.controller.esurfingclient", package.seeall)

function index()
    entry({"admin", "esurfingclient", "read_config"}, call("action_read_config")).leaf = true
    entry({"admin", "esurfingclient", "write_config"}, call("action_write_config")).leaf = true
    entry({"admin", "esurfingclient", "restart_service"}, call("action_restart_service")).leaf = true
end

function action_read_config()
    local file = io.open("/etc/config/esurfingclient", "r")
    if not file then
        luci.http.prepare_content("application/json")
        luci.http.write('{"error":"Cannot open file"}')
        return
    end
    local content = file:read("*a")
    file:close()
    luci.http.prepare_content("application/json")
    luci.http.write(content)
end

function action_write_config()
    local data = luci.http.content()
    if not data then
        luci.http.status(400, "Bad Request")
        return
    end
    local file = io.open("/etc/config/esurfingclient", "w")
    if not file then
        luci.http.status(500, "Internal Server Error")
        return
    end
    file:write(data)
    file:close()
    luci.http.prepare_content("application/json")
    luci.http.write('{"status":"ok"}')
end

function action_restart_service()
    os.execute("/etc/init.d/esurfingclient restart >/dev/null 2>&1")
    luci.http.prepare_content("application/json")
    luci.http.write('{"status":"ok"}')
end
