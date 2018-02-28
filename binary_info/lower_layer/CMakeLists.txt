#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

add_analytic_directory(mqtt_transport_ll "binary_info/lower_layer")
add_analytic_directory(mqtt_ws_transport_ll "binary_info/lower_layer")
add_analytic_directory(amqp_transport_ll "binary_info/lower_layer")
add_analytic_directory(amqp_ws_transport_ll "binary_info/lower_layer")
add_analytic_directory(http_transport_ll "binary_info/lower_layer")

if (${use_prov_client})
    add_analytic_directory(prov_mqtt_transport_ll "binary_info/lower_layer")
    add_analytic_directory(prov_mqtt_ws_transport_ll "binary_info/lower_layer")
    add_analytic_directory(prov_amqp_ws_transport_ll "binary_info/lower_layer")
    add_analytic_directory(prov_amqp_transport_ll "binary_info/lower_layer")
    add_analytic_directory(prov_http_transport_ll "binary_info/lower_layer")
endif()
