package com.example.plantmonitor

import org.junit.Test
import org.junit.Assert.*
import java.util.regex.Pattern

class ExampleUnitTest {
    @Test
    fun htmlParsing_isCorrect() {
        val html = "<div class='dado'>\uFE0F Temperatura: <strong>23.5 °C</strong></div>" +
                   "<div class='dado'>\u2600\uFE0F Luminosidade: <strong>100.0 lx</strong></div>" +
                   "<div class='dado'>\uD83D\uDCA7 Humidade: <strong>50 %</strong></div>"

        val tempMatcher = Pattern.compile("Temperatura: <strong>([-0-9.]+) °C</strong>").matcher(html)
        val lightMatcher = Pattern.compile("Luminosidade: <strong>([0-9.]+) lx</strong>").matcher(html)
        val humMatcher = Pattern.compile("Humidade: <strong>([0-9.]+) %</strong>").matcher(html)

        assertTrue(tempMatcher.find())
        assertEquals("23.5", tempMatcher.group(1))

        assertTrue(lightMatcher.find())
        assertEquals("100.0", lightMatcher.group(1))

        assertTrue(humMatcher.find())
        assertEquals("50", humMatcher.group(1))
    }
}
