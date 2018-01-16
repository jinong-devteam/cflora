-- phpMyAdmin SQL Dump
-- version 4.2.12deb2+deb8u2
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Mar 06, 2017 at 08:24 PM
-- Server version: 5.5.52-0+deb8u1
-- PHP Version: 5.6.27-0+deb8u1

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+09:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `farmos`
--
CREATE DATABASE IF NOT EXISTS `farmos` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;
USE `farmos`;


--
-- Table structure for table `gos_configuration`
--

DROP TABLE IF EXISTS `gos_configuration`;
CREATE TABLE IF NOT EXISTS `gos_configuration` (
  `restart` int(11) NOT NULL DEFAULT '0',
  `control` text NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_configuration`
--

TRUNCATE TABLE `gos_configuration`;
--
-- Dumping data for table `gos_configuration`
--

INSERT INTO `gos_configuration` (`restart`, `control`) VALUES
(0, 'manual');

-- --------------------------------------------------------

--
-- Table structure for table `gos_control`
--

DROP TABLE IF EXISTS `gos_control`;
CREATE TABLE IF NOT EXISTS `gos_control` (
  `id` int(11) NOT NULL,
  `exectime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `device_id` int(11) NOT NULL,
  `argument` int(11) NOT NULL DEFAULT '-1',
  `ctrltype` text NOT NULL,
  `ctrlid` int(11) DEFAULT NULL,
  `ctrlarg` int(11) NOT NULL DEFAULT '-1',
  `worktime` int(11) NOT NULL DEFAULT '-1',
  `sentcnt` int(11) NOT NULL DEFAULT '0',
  `stopcnt` int(11) NOT NULL DEFAULT '0',
  `finishtime` datetime DEFAULT NULL,
  `status` int(11) NOT NULL DEFAULT '0',
  `senttime` datetime DEFAULT NULL
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_control`
--

TRUNCATE TABLE `gos_control`;
-- --------------------------------------------------------

--
-- Table structure for table `gos_control_history`
--

DROP TABLE IF EXISTS `gos_control_history`;
CREATE TABLE IF NOT EXISTS `gos_control_history` (
  `id` int(11) NOT NULL,
  `exectime` datetime NOT NULL,
  `device_id` int(11) NOT NULL,
  `argument` int(11) DEFAULT NULL,
  `ctrltype` text NOT NULL,
  `ctrlid` int(11) DEFAULT NULL,
  `ctrlarg` int(11) NOT NULL DEFAULT '-1',
  `worktime` int(11) NOT NULL,
  `sentcnt` int(11) NOT NULL DEFAULT '0',
  `stopcnt` int(11) NOT NULL DEFAULT '0',
  `finishtime` datetime DEFAULT NULL,
  `status` int(11) NOT NULL DEFAULT '0',
  `senttime` datetime NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_control_history`
--

TRUNCATE TABLE `gos_control_history`;
--
-- Dumping data for table `gos_control_history`
--

-- --------------------------------------------------------

--
-- Table structure for table `gos_control_rule`
--

DROP TABLE IF EXISTS `gos_control_rule`;
CREATE TABLE IF NOT EXISTS `gos_control_rule` (
  `id` int(11) NOT NULL,
  `priority` int(11) NOT NULL,
  `period` int(11) NOT NULL,
  `field_id` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_control_rule`
--

TRUNCATE TABLE `gos_control_rule`;
-- --------------------------------------------------------

--
-- Table structure for table `gos_control_rule_action`
--

DROP TABLE IF EXISTS `gos_control_rule_action`;
CREATE TABLE IF NOT EXISTS `gos_control_rule_action` (
  `seq` int(11) NOT NULL,
  `id` int(11) NOT NULL,
  `istrue` int(11) NOT NULL,
  `actuator_id` int(11) NOT NULL,
  `argument` int(11) DEFAULT NULL,
  `workingtime` int(11) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_control_rule_action`
--

TRUNCATE TABLE `gos_control_rule_action`;
-- --------------------------------------------------------

--
-- Table structure for table `gos_control_rule_condition`
--

DROP TABLE IF EXISTS `gos_control_rule_condition`;
CREATE TABLE IF NOT EXISTS `gos_control_rule_condition` (
  `seq` int(11) NOT NULL,
  `id` int(11) NOT NULL,
  `operator` text NOT NULL,
  `sensor_id` int(11) NOT NULL,
  `ftime` int(11) NOT NULL,
  `ttime` int(11) NOT NULL,
  `fvalue` double NOT NULL,
  `tvalue` double NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_control_rule_condition`
--

TRUNCATE TABLE `gos_control_rule_condition`;
-- --------------------------------------------------------

--
-- Table structure for table `gos_devicemap`
--

DROP TABLE IF EXISTS `gos_devicemap`;
CREATE TABLE IF NOT EXISTS `gos_devicemap` (
  `field_id` int(11) NOT NULL,
  `device_id` int(11) NOT NULL,
  `name` text NOT NULL,
  `unit` varchar(10) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_devicemap`
--

TRUNCATE TABLE `gos_devicemap`;
--
-- Dumping data for table `gos_devicemap`
--

-- --------------------------------------------------------

--
-- Table structure for table `gos_devices`
--

DROP TABLE IF EXISTS `gos_devices`;
CREATE TABLE IF NOT EXISTS `gos_devices` (
  `id` int(11) NOT NULL,
  `devtype` text NOT NULL,
  `subtype` text NOT NULL,
  `status` text NOT NULL,
  `catalog_id` int(11) DEFAULT NULL,
  `gcg_id` int(11) DEFAULT NULL,
  `node_id` int(11) DEFAULT NULL,
  `dev_id` int(11) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_devices`
--

TRUNCATE TABLE `gos_devices`;
--
-- Dumping data for table `gos_devices`
--

-- --------------------------------------------------------

--
-- Table structure for table `gos_device_convertmap`
--

DROP TABLE IF EXISTS `gos_device_convertmap`;
CREATE TABLE IF NOT EXISTS `gos_device_convertmap` (
  `device_id` int(11) NOT NULL,
  `ctype` text NOT NULL,
  `configuration` text NOT NULL,
  `offset` double DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_device_convertmap`
--

TRUNCATE TABLE `gos_device_convertmap`;
--
-- Dumping data for table `gos_device_convertmap`
--

-- --------------------------------------------------------

--
-- Table structure for table `gos_device_history`
--

DROP TABLE IF EXISTS `gos_device_history`;
CREATE TABLE IF NOT EXISTS `gos_device_history` (
  `device_id` int(11) NOT NULL,
  `updatetime` datetime NOT NULL,
  `rawvalue` int(11) DEFAULT NULL,
  `argument` int(11) DEFAULT NULL,
  `worktime` int(11) DEFAULT NULL,
  `control_id` int(11) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_device_history`
--

TRUNCATE TABLE `gos_device_history`;
--
-- Dumping data for table `gos_device_history`
--

-- --------------------------------------------------------

--
-- Table structure for table `gos_device_portmap`
--

DROP TABLE IF EXISTS `gos_device_portmap`;
CREATE TABLE IF NOT EXISTS `gos_device_portmap` (
  `device_id` int(11) NOT NULL,
  `board_id` int(11) NOT NULL,
  `ptype` text NOT NULL,
  `channel` int(11) NOT NULL,
  `name` text,
  `opt` text,
  `arg` int(11) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_device_portmap`
--

TRUNCATE TABLE `gos_device_portmap`;
--
-- Dumping data for table `gos_device_portmap`
--

-- --------------------------------------------------------

--
-- Table structure for table `gos_device_properties`
--

DROP TABLE IF EXISTS `gos_device_properties`;
CREATE TABLE IF NOT EXISTS `gos_device_properties` (
  `device_id` int(11) NOT NULL,
  `propkey` text NOT NULL,
  `propvalue` text NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_device_properties`
--

TRUNCATE TABLE `gos_device_properties`;
--
-- Dumping data for table `gos_device_properties`
--

-- --------------------------------------------------------

--
-- Table structure for table `gos_device_status`
--

DROP TABLE IF EXISTS `gos_device_status`;
CREATE TABLE IF NOT EXISTS `gos_device_status` (
  `device_id` int(11) NOT NULL,
  `updatetime` datetime NOT NULL,
  `rawvalue` int(11) DEFAULT NULL,
  `argument` int(11) DEFAULT NULL,
  `worktime` int(11) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_device_status`
--

TRUNCATE TABLE `gos_device_status`;
--
-- Dumping data for table `gos_device_status`
--


-- --------------------------------------------------------

--
-- Table structure for table `gos_environment`
--

CREATE TABLE IF NOT EXISTS `gos_environment` (
  `obstime` datetime NOT NULL,
  `device_id` int(11) NOT NULL,
  `nvalue` double DEFAULT NULL,
  `rawvalue` int(11) NOT NULL,
  `field_id` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Dumping data for table `gos_environment`
--


-- --------------------------------------------------------

--
-- Table structure for table `gos_environment_current`
--

DROP TABLE IF EXISTS `gos_environment_current`;
CREATE TABLE IF NOT EXISTS `gos_environment_current` (
  `obstime` datetime NOT NULL,
  `device_id` int(11) NOT NULL,
  `nvalue` double DEFAULT NULL,
  `rawvalue` int(11) NOT NULL,
  `field_id` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_environment_current`
--

TRUNCATE TABLE `gos_environment_current`;
--
-- Dumping data for table `gos_environment_current`
--

-- --------------------------------------------------------

--
-- Table structure for table `gos_farm`
--

DROP TABLE IF EXISTS `gos_farm`;
CREATE TABLE IF NOT EXISTS `gos_farm` (
  `name` text NOT NULL,
  `address` text,
  `postcode` text,
  `telephone` text,
  `owner` text,
  `maincrop` text
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_farm`
--

TRUNCATE TABLE `gos_farm`;
--
-- Dumping data for table `gos_farm`
--

-- --------------------------------------------------------

--
-- Table structure for table `gos_fields`
--

DROP TABLE IF EXISTS `gos_fields`;
CREATE TABLE IF NOT EXISTS `gos_fields` (
  `id` int(11) NOT NULL,
  `name` text NOT NULL,
  `fieldtype` text NOT NULL,
  `latitude` double NOT NULL,
  `longitude` double NOT NULL,
  `crop` text,
  `direction` text
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Truncate table before insert `gos_fields`
--

TRUNCATE TABLE `gos_fields`;
--
-- Dumping data for table `gos_fields`
--


-- --------------------------------------------------------

--
-- Indexes for table `gos_control`
--
ALTER TABLE `gos_control`
 ADD PRIMARY KEY (`id`), ADD KEY `device_id` (`device_id`);

--
-- Indexes for table `gos_control_history`
--
ALTER TABLE `gos_control_history`
 ADD PRIMARY KEY (`id`), ADD KEY `device_id` (`device_id`);

--
-- Indexes for table `gos_control_rule`
--
ALTER TABLE `gos_control_rule`
 ADD PRIMARY KEY (`id`), ADD KEY `field_id` (`field_id`);

--
-- Indexes for table `gos_control_rule_action`
--
ALTER TABLE `gos_control_rule_action`
 ADD PRIMARY KEY (`seq`), ADD KEY `id` (`id`), ADD KEY `actuator_id` (`actuator_id`);

--
-- Indexes for table `gos_control_rule_condition`
--
ALTER TABLE `gos_control_rule_condition`
 ADD PRIMARY KEY (`seq`), ADD KEY `id` (`id`), ADD KEY `sensor_id` (`sensor_id`);

--
-- Indexes for table `gos_devicemap`
--
ALTER TABLE `gos_devicemap`
 ADD KEY `field_id` (`field_id`), ADD KEY `device_id` (`device_id`);

--
-- Indexes for table `gos_devices`
--
ALTER TABLE `gos_devices`
 ADD PRIMARY KEY (`id`);

-- Indexes for table `gos_device_convertmap`
--
ALTER TABLE `gos_device_convertmap`
 ADD KEY `device_id` (`device_id`);

--
-- Indexes for table `gos_device_history`
--
ALTER TABLE `gos_device_history`
 ADD KEY `device_id` (`device_id`);

--
-- Indexes for table `gos_device_portmap`
--
ALTER TABLE `gos_device_portmap`
 ADD KEY `device_id` (`device_id`), ADD KEY `board_id` (`board_id`);

--
-- Indexes for table `gos_device_status`
--
ALTER TABLE `gos_device_status`
 ADD KEY `device_id` (`device_id`);

--
-- Indexes for table `gos_environment`
--
ALTER TABLE `gos_environment`
 ADD KEY `device_id` (`device_id`);

--
-- Constraints for table `gos_environment`
--
ALTER TABLE `gos_environment`
ADD CONSTRAINT `gos_environment_ibfk_1` FOREIGN KEY (`device_id`) REFERENCES `gos_devices` (`id`);
--
-- Indexes for table `gos_environment_current`
--
ALTER TABLE `gos_environment_current`
 ADD KEY `device_id` (`device_id`);

--
-- Indexes for table `gos_fields`
--
ALTER TABLE `gos_fields`
 ADD PRIMARY KEY (`id`);


--
-- AUTO_INCREMENT for table `gos_control`
--
ALTER TABLE `gos_control`
MODIFY `id` int(11) NOT NULL AUTO_INCREMENT,AUTO_INCREMENT=1;
--
-- Constraints for dumped tables
--

--
-- Constraints for table `gos_control`
--
ALTER TABLE `gos_control`
ADD CONSTRAINT `gos_control_ibfk_1` FOREIGN KEY (`device_id`) REFERENCES `gos_devices` (`id`);

--
-- Constraints for table `gos_control_history`
--
ALTER TABLE `gos_control_history`
ADD CONSTRAINT `gos_control_history_ibfk_1` FOREIGN KEY (`device_id`) REFERENCES `gos_devices` (`id`);

--
-- Constraints for table `gos_control_rule`
--
ALTER TABLE `gos_control_rule`
ADD CONSTRAINT `gos_control_rule_ibfk_1` FOREIGN KEY (`field_id`) REFERENCES `gos_fields` (`id`) ON DELETE CASCADE;

--
-- Constraints for table `gos_control_rule_action`
--
ALTER TABLE `gos_control_rule_action`
ADD CONSTRAINT `gos_control_rule_action_ibfk_1` FOREIGN KEY (`id`) REFERENCES `gos_control_rule` (`id`),
ADD CONSTRAINT `gos_control_rule_action_ibfk_2` FOREIGN KEY (`actuator_id`) REFERENCES `gos_devices` (`id`);

--
-- Constraints for table `gos_control_rule_condition`
--
ALTER TABLE `gos_control_rule_condition`
ADD CONSTRAINT `gos_control_rule_condition_ibfk_1` FOREIGN KEY (`id`) REFERENCES `gos_control_rule` (`id`),
ADD CONSTRAINT `gos_control_rule_condition_ibfk_2` FOREIGN KEY (`sensor_id`) REFERENCES `gos_devices` (`id`);

--
-- Constraints for table `gos_devicemap`
--
-- ALTER TABLE `gos_devicemap`
-- ADD CONSTRAINT `gos_devicemap_ibfk_1` FOREIGN KEY (`field_id`) REFERENCES `gos_fields` (`id`) ON DELETE CASCADE,
-- ADD CONSTRAINT `gos_devicemap_ibfk_2` FOREIGN KEY (`device_id`) REFERENCES `gos_devices` (`id`) ON DELETE CASCADE;

--
-- Constraints for table `gos_device_convertmap`
--
ALTER TABLE `gos_device_convertmap`
ADD CONSTRAINT `gos_device_convertmap_ibfk_1` FOREIGN KEY (`device_id`) REFERENCES `gos_devices` (`id`) ON DELETE CASCADE;

--
-- Constraints for table `gos_device_history`
--
ALTER TABLE `gos_device_history`
ADD CONSTRAINT `gos_device_history_ibfk_1` FOREIGN KEY (`device_id`) REFERENCES `gos_devices` (`id`);

--
-- Constraints for table `gos_device_portmap`
--
ALTER TABLE `gos_device_portmap`
ADD CONSTRAINT `gos_device_portmap_ibfk_1` FOREIGN KEY (`device_id`) REFERENCES `gos_devices` (`id`) ON DELETE CASCADE,
ADD CONSTRAINT `gos_device_portmap_ibfk_2` FOREIGN KEY (`board_id`) REFERENCES `gos_boards` (`id`) ON DELETE CASCADE;

--
-- Constraints for table `gos_device_status`
--
ALTER TABLE `gos_device_status`
ADD CONSTRAINT `gos_device_status_ibfk_1` FOREIGN KEY (`device_id`) REFERENCES `gos_devices` (`id`);

DROP EVENT `monthly_backup`;

DELIMITER $$

CREATE EVENT `monthly_backup`
    ON SCHEDULE EVERY 1 MONTH STARTS '2017-05-01 00:00:01'
    DO BEGIN
        SET @query = concat('CREATE TABLE IF NOT EXISTS gos_environment_', date_format (now(), '%Y%m'), '(obstime datetime NOT NULL,device_id int(11) NOT NULL,nvalue double DEFAULT NULL,', 'rawvalue int(11) NOT NULL,field_id int(11) NOT NULL) ENGINE=InnoDB DEFAULT CHARSET=utf8');

        PREPARE stmt1 FROM @query;
        EXECUTE stmt1;
        DEALLOCATE PREPARE stmt1;

        SET @query = concat('alter table gos_environment_', date_format (now(), '%Y%m'), ' add index (device_id, obstime)');

        PREPARE stmt1 FROM @query;
        EXECUTE stmt1;
        DEALLOCATE PREPARE stmt1;

END
$$
DELIMITER ;

SET GLOBAL event_scheduler = ON;

DROP EVENT `daily_backup`;

DELIMITER $$

CREATE EVENT `daily_backup` 
    ON SCHEDULE EVERY 1 DAY STARTS '2017-04-12 01:00:00' 
    DO BEGIN
        SET @query = concat('CREATE TABLE IF NOT EXISTS gos_environment_', date_format (now(), '%Y%m'), '(obstime datetime NOT NULL,device_id int(11) NOT NULL,nvalue double DEFAULT NULL,', 'rawvalue int(11) NOT NULL,field_id int(11) NOT NULL) ENGINE=InnoDB DEFAULT CHARSET=utf8');

        PREPARE stmt1 FROM @query; 
        EXECUTE stmt1; 
        DEALLOCATE PREPARE stmt1; 

        SET @query = concat('insert into gos_environment_', date_format (curdate() - INTERVAL 2 DAY, '%Y%m'), '(device_id, obstime, nvalue, rawvalue) select device_id, obstime, nvalue, rawvalue from gos_environment ', 'where obstime < curdate() - INTERVAL 1 DAY');

        PREPARE stmt1 FROM @query; 
        EXECUTE stmt1; 
        DEALLOCATE PREPARE stmt1; 

        Delete from gos_environment where obstime < curdate() - INTERVAL 1 DAY;

END 
$$
DELIMITER ;

SET GLOBAL event_scheduler = ON;

--
-- Constraints for table `gos_environment_current`
--
-- ALTER TABLE `gos_environment_current`
-- ADD CONSTRAINT `gos_environment_current_ibfk_1` FOREIGN KEY (`device_id`) REFERENCES `gos_devices` (`id`);

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;


