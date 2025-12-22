/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

import QtQuick 2.15
import QtTest 1.15
import UtilsQt 1.0

Item {
    id: root
    width: 200
    height: 200

    // Test MaskInverter with a Rectangle child
    MaskInverter {
        id: maskInverter
        width: 100
        height: 100

        Rectangle {
            id: innerRect
            x: 20
            y: 20
            width: 60
            height: 60
        }
    }

    // Test MaskInvertedRoundedRect
    MaskInvertedRoundedRect {
        id: maskInvertedRoundedRect
        width: 100
        height: 100
        radius: 20
    }

    TestCase {
        name: "MaskInverterTest"

        function test_00_outsideChild_returnsTrue() {
            // Point outside the inner rectangle should return true (inverted)
            verify(maskInverter.contains(Qt.point(5, 5)));
            verify(maskInverter.contains(Qt.point(90, 90)));
            verify(maskInverter.contains(Qt.point(15, 50)));
        }

        function test_01_insideChild_returnsFalse() {
            // Point inside the inner rectangle should return false (inverted)
            verify(!maskInverter.contains(Qt.point(50, 50)));
            verify(!maskInverter.contains(Qt.point(25, 25)));
            verify(!maskInverter.contains(Qt.point(75, 75)));
        }

        function test_02_onChildEdge_returnsFalse() {
            // Point on the edge of inner rectangle
            // Rectangle spans from (20,20) to (79,79) since width=60, height=60
            verify(!maskInverter.contains(Qt.point(20, 20)));
            verify(!maskInverter.contains(Qt.point(79, 79)));
        }
    }

    TestCase {
        name: "MaskInvertedRoundedRectTest"

        function test_00_centerArea_returnsFalse() {
            // Points in center (not in corners) should return false
            verify(!maskInvertedRoundedRect.contains(Qt.point(50, 50)));
            verify(!maskInvertedRoundedRect.contains(Qt.point(50, 10)));
            verify(!maskInvertedRoundedRect.contains(Qt.point(10, 50)));
        }

        function test_01_cornerOutsideCircle_returnsTrue() {
            // Points in corners outside the rounded area should return true
            verify(maskInvertedRoundedRect.contains(Qt.point(2, 2)));
            verify(maskInvertedRoundedRect.contains(Qt.point(98, 2)));
            verify(maskInvertedRoundedRect.contains(Qt.point(2, 98)));
            verify(maskInvertedRoundedRect.contains(Qt.point(98, 98)));
        }

        function test_02_cornerInsideCircle_returnsFalse() {
            // Points in corners but inside the rounded area should return false
            verify(!maskInvertedRoundedRect.contains(Qt.point(15, 15)));
            verify(!maskInvertedRoundedRect.contains(Qt.point(85, 15)));
            verify(!maskInvertedRoundedRect.contains(Qt.point(15, 85)));
            verify(!maskInvertedRoundedRect.contains(Qt.point(85, 85)));
        }

        function test_03_edgeOfRoundedCorner() {
            // Point exactly on the corner circle edge (approximately)
            // At radius 20, corner center is at (20,20)
            // Point at ~(6, 6) should be outside: distance = sqrt(14^2 + 14^2) ≈ 19.8 < 20
            verify(!maskInvertedRoundedRect.contains(Qt.point(6, 6)));
            // Point at ~(3, 3) should be outside circle: distance = sqrt(17^2 + 17^2) ≈ 24 > 20
            verify(maskInvertedRoundedRect.contains(Qt.point(3, 3)));
        }

        function test_04_zeroRadius_alwaysFalse() {
            maskInvertedRoundedRect.radius = 0;
            verify(!maskInvertedRoundedRect.contains(Qt.point(2, 2)));
            verify(!maskInvertedRoundedRect.contains(Qt.point(50, 50)));
            maskInvertedRoundedRect.radius = 20; // Restore
        }

        function test_05_radiusProperty() {
            compare(maskInvertedRoundedRect.radius, 20);
            maskInvertedRoundedRect.radius = 30;
            compare(maskInvertedRoundedRect.radius, 30);
            maskInvertedRoundedRect.radius = 20; // Restore
        }
    }
}
