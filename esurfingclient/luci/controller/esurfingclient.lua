module("luci.controller.esurfingclient", package.seeall)

local json = require "luci.jsonc"

function index()
    entry({"admin", "esurfingclient", "read_config"}, call("action_read_config")).leaf = true
    entry({"admin", "esurfingclient", "write_config"}, call("action_write_config")).leaf = true
    entry({"admin", "esurfingclient", "restart_service"}, call("action_restart_service")).leaf = true
end

function safe_str(val)
    if type(val) == "string" then return val else return "" end
end

function account_to_json(acc)
    if type(acc) ~= "table" then acc = {} end
    return string.format(
            '{"username":%s,"password":%s,"channel":%s,"mark":%s}',
            json.stringify(safe_str(acc.username)),
            json.stringify(safe_str(acc.password)),
            json.stringify(safe_str(acc.channel == "pc" and "pc" or "phone")),
            json.stringify(safe_str(acc.mark))
    )
end

function config_to_json(config)
    local accounts = {}
    if type(config) == "table" and type(config.accounts) == "table" then
        for _, acc in ipairs(config.accounts) do
            accounts[#accounts + 1] = account_to_json(acc)
        end
    end
    if #accounts == 0 then
        accounts[#accounts + 1] = account_to_json({})
    end

    return string.format(
            '{\n  "enabled": %s,\n  "log_lv": %d,\n  "accounts": [\n    %s\n  ]\n}',
            (type(config) == "table" and config.enabled == true) and "true" or "false",
            (type(config) == "table" and type(config.log_lv) == "number") and config.log_lv or 4,
            table.concat(accounts, ",\n    ")
    )
end

function action_read_config()
    local file = io.open("/etc/config/esurfingclient", "r")
    local content
    if file then
        content = file:read("*a")
        file:close()
    end

    local config
    if content then
        local ok, parsed = pcall(json.parse, content)
        if ok then config = parsed end
    end

    luci.http.prepare_content("application/json")
    luci.http.write(config_to_json(config))
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