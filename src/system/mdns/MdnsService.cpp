//
// Created by long.nguyenviet on 04/08/2024.
//

#include "MdnsService.h"

MdnsService::MdnsService() {

}

void MdnsService::setup() {
    mdns_init();
    mdns_hostname_set("my_esp8266");
    mdns_instance_name_set("My ESP8266 device");
}

RegisterServiceAddress* MdnsService::lookUp() {
    RegisterServiceAddress* service = nullptr;
    mdns_result_t *results = NULL;
    // Lookup for the HTTP service (_http._tcp) on the network
    esp_err_t err = mdns_query_ptr("_http", "_tcp", 3000, 10, &results);
    if (err) {
        esp_logi(MDNS, "Query failed: %s", esp_err_to_name(err));
        return service;
    }

    if (!results) {
        esp_logi(MDNS, "No services found!");
        return service;
    }

    mdns_result_t *r = results;
    while (r) {
        if (r->addr && r->addr->addr.type == IPADDR_TYPE_V4) {
            esp_logi(MDNS, "Found service on IP: %s --> port: %d", ip4addr_ntoa(&r->addr->addr.u_addr.ip4), r->port);
            service = new RegisterServiceAddress(&r->addr->addr.u_addr.ip4, r->port);
            break;
        } else {
            esp_logi(MDNS, "Invalid IP address in mDNS result");
        }
        r = r->next;
    }

    mdns_query_results_free(results);
    return service;
}