import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import QtLocation 5.3
import QtPositioning 5.8
import QtQuick.Window 2.2

import ru.indigosystem.mobile 1.0

ApplicationWindow {
    visible: true
    width: Screen.desktopAvailableWidth
    height: Screen.desktopAvailableHeight
    title: qsTr("Hello World")
    id: mainWindow

    ScoroNetwork {
        id: scoronetwork

        onRoutePointsChanged: {
            console.log("route points received!")
            routePolyline.path = []
            var path = []
            for (var i = 0; i < Object.keys(routePoints).length; i++) {
                var key = String(i)
                path.push(QtPositioning.coordinate(
                                                routePoints[key][1],
                                                routePoints[key][0]))
            }
            busUpdateTimer.running = true
            routePolyline.path = path
            routePolyline.update()
        }

        onRouteNameChanged: {
            console.log("new route name: " + routeName)
            scoronetwork.getRoutesAndPoints()
        }
    }

    Plugin {
        id: osmPlugin
        name: "osm"
        // specify plugin parameters if necessary
        // PluginParameter {
        //     name:
        //     value:
        // }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: osmPlugin
        center: QtPositioning.coordinate(47.229229, 38.912398) // Taganrog
        zoomLevel: 12

        Row {
            id: topButtons

            Button {
                padding: 5
                width: mainWindow.width / 5
                id: route2
                text: "2"

                onClicked: {
                    scoronetwork.routeName = "2"
                }
            }

            Button {
                padding: 5
                width: mainWindow.width / 5
                id: route5
                text: "5"

                onClicked: {
                    scoronetwork.routeName = "5"
                }
            }

            Button {
                padding: 5
                width: mainWindow.width / 5
                id: route60
                text: "60"
                onClicked: {
                    scoronetwork.routeName = "60"
                }
            }

            Button {
                padding: 5
                width: mainWindow.width / 5
                id: route74
                text: "74"
                onClicked: {
                    scoronetwork.routeName = "74"
                }
            }

        }


        MapPolyline {
            id: routePolyline
            line.width: 3
            line.color: 'green'
            path: []
        }

        MapItemView {
            id: busesView
            model: scoronetwork.busModel
            delegate: MapCircle {
                id: point
                radius: 100
                color: "red"
                border.color: "black"
                border.width: 2
                smooth: true
                opacity: 0.8
                center: QtPositioning.coordinate(latitude, longitude)
            }
        }

        /*
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onDoubleClicked: {
                var mouseGeoPos = map.toCoordinate(Qt.point(mouse.x, mouse.y))
                var preZoomPoint = map.fromCoordinate(mouseGeoPos, false)
                if (mouse.button === Qt.LeftButton) {
                    map.zoomLevel += 2
                } else if (mouse.button === Qt.RightButton) {
                    map.zoomLevel -= 2
                }
                var postZoomPoint = map.fromCoordinate(mouseGeoPos, false)
                var dx = postZoomPoint.x - preZoomPoint.x
                var dy = postZoomPoint.y - preZoomPoint.y

                var mapCenterPoint = Qt.point(map.width / 2.0 + dx,
                                              map.height / 2.0 + dy)
                map.center = map.toCoordinate(mapCenterPoint)
            }
        }
        */
    }

        function init() {
            console.log("init called")
            scoronetwork.routeName = "2"
        }

        Timer {
            id: busUpdateTimer
            interval: 5000
            repeat: true
            triggeredOnStart: true
            onTriggered: {
                //console.log("updateTimerTick")
                scoronetwork.updateTimerTick()
            }
        }
    }

