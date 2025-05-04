#include "main.h"
#include <algorithm>
#include "pros/abstract_motor.hpp"

/////
// For installation, upgrading, documentations, and tutorials, check out our website!
// https://ez-robotics.github.io/EZ-Template/
/////

// Chassis constructor
ez::Drive chassis (
    {-7, -18, 19},     // Left Chassis Ports (negative port will reverse it!)
    {9, 4, -8},  // Right Chassis Ports (negative port will reverse it!)

    6,      // IMU Port
    3.25,  // Wheel Diameter (Remember, 4" wheels without screw holes are actually 4.125!)
    450);   // Wheel RPM = cartridge * (motor gear / wheel gear)

/*clamp Variables*/
bool lastKnownButtonBState;
int ClampState = 0; /*0 = off, 1 = Grab*/


extern pros::Motor Intake1;
extern pros::Motor Intake2;
extern pros::Motor LB;
extern pros::Controller master;

extern pros::adi::DigitalOut MOGO;
extern pros::adi::DigitalOut arm;
pros::Motor Intake1(2);
//pros::Motor Intake2(11);8
pros::Motor LB (1);
pros::Controller master (pros::E_CONTROLLER_MASTER);
    #define MOGO_PORT 'b'
    #define ARM_PORT 'c'
    #define intakep 'a'
pros::adi::DigitalOut MOGO(MOGO_PORT);
pros::adi::DigitalOut arm(ARM_PORT);
pros::adi::DigitalOut PNUEINT(intakep);

ez::PID liftPID{0.45, 0, 0, 0, "Lift"};

const int DRIVE_SPEED = 127;
const int TURN_SPEED = 87;
const int SWING_SPEED = 100;

inline void set_lift(int input) {
    LB.move(input);
    LB.move(input);
  }

// Uncomment the trackers you're using here!
// - `8` and `9` are smart ports (making these negative will reverse the sensor)
//  - you should get positive values on the encoders going FORWARD and RIGHT
// - `2.75` is the wheel diameter
// - `4.0` is the distance from the center of the wheel to the center of the robot
// ez::tracking_wheel horiz_tracker(8, 2.75, 4.0);  // This tracking wheel is perpendicular to the drive wheels
// ez::tracking_wheel vert_tracker(9, 2.75, 4.0);   // This tracking wheel is parallel to the drive wheels

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {
  // Print our branding over your terminal :D
  ez::ez_template_print();

  pros::delay(500);  // Stop the user from doing anything while legacy ports configure

  // Look at your horizontal tracking wheel and decide if it's in front of the midline of your robot or behind it
  //  - change `back` to `front` if the tracking wheel is in front of the midline
  //  - ignore this if you aren't using a horizontal tracker
  // chassis.odom_tracker_back_set(&horiz_tracker);
  // Look at your vertical tracking wheel and decide if it's to the left or right of the center of the robot
  //  - change `left` to `right` if the tracking wheel is to the right of the centerline
  //  - ignore this if you aren't using a vertical tracker
  // chassis.odom_tracker_left_set(&vert_tracker);

  // Configure your chassis controls
  chassis.opcontrol_curve_buttons_toggle(true);   // Enables modifying the controller curve with buttons on the joysticks
  chassis.opcontrol_drive_activebrake_set(0.0);   // Sets the active brake kP. We recommend ~2.  0 will disable.
  chassis.opcontrol_curve_default_set(7.0, 0.0);  // Defaults for curve. If using tank, only the first parameter is used. (Comment this line out if you have an SD card!)

  // Set the drive to your own constants from autons.cpp!
  default_constants();

  // These are already defaulted to these buttons, but you can change the left/right curve buttons here!
  // chassis.opcontrol_curve_buttons_left_set(pros::E_CONTROLLER_DIGITAL_LEFT, pros::E_CONTROLLER_DIGITAL_RIGHT);  // If using tank, only the left side is used.
  // chassis.opcontrol_curve_buttons_right_set(pros::E_CONTROLLER_DIGITAL_Y, pros::E_CONTROLLER_DIGITAL_A);

  // // Autonomous Selector using LLEMU
  // ez::as::auton_selector.autons_add({
  //     {"Drive\n\nDrive forward and come back", drive_example},
  //     {"Turn\n\nTurn 3 times.", turn_example},
  //     {"Drive and Turn\n\nDrive forward, turn, come back", drive_and_turn},
  //     {"Drive and Turn\n\nSlow down during drive", wait_until_change_speed},
  //     {"Swing Turn\n\nSwing in an 'S' curve", swing_example},
  //     {"Motion Chaining\n\nDrive forward, turn, and come back, but blend everything together :D", motion_chaining},
  //     {"Combine all 3 movements", combining_movements},
  //     {"Interference\n\nAfter driving forward, robot performs differently if interfered or not", interfered_example},
  //     {"Simple Odom\n\nThis is the same as the drive example, but it uses odom instead!", odom_drive_example},
  //     {"Pure Pursuit\n\nGo to (0, 30) and pass through (6, 10) on the way.  Come back to (0, 0)", odom_pure_pursuit_example},
  //     {"Pure Pursuit Wait Until\n\nGo to (24, 24) but start running an intake once the robot passes (12, 24)", odom_pure_pursuit_wait_until_example},
  //     {"Boomerang\n\nGo to (0, 24, 45) then come back to (0, 0, 0)", odom_boomerang_example},
  //     {"Boomerang Pure Pursuit\n\nGo to (0, 24, 45) on the way to (24, 24) then come back to (0, 0, 0)", odom_boomerang_injected_pure_pursuit_example},
  //     {"Measure Offsets\n\nThis will turn the robot a bunch of times and calculate your offsets for your tracking wheels.", measure_offsets},
  // });

  // Initialize chassis and auton selector
  chassis.initialize();
  ez::as::initialize();
  master.rumble(chassis.drive_imu_calibrated() ? "." : "---");
  liftPID.exit_condition_set(80, 50, 300, 150, 500, 500);

}

void lift_auto(double target) {
  liftPID.target_set(target);
  while (liftPID.exit_condition(LB, true) == ez::RUNNING) {
    double output = liftPID.compute(LB.get_position());
    set_lift(output);
    pros::delay(ez::util::DELAY_TIME);
  }
  set_lift(0);
}

void lift_task() {
  pros::delay(2000);  // Set EZ-Template calibrate before this function starts running
  while (true) {
    set_lift(liftPID.compute(LB.get_position()));

    pros::delay(ez::util::DELAY_TIME);
  }
}
pros::Task Lift_Task(lift_task);  // Create the task, this will cause the function to start running

inline void lift_wait() {
  while (liftPID.exit_condition(LB, true) == ez::RUNNING) {
    pros::delay(ez::util::DELAY_TIME);
  }
}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {
  // . . .
}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {
  // . . .
}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
void autonomous() {
  chassis.pid_targets_reset();                // Resets PID targets to 0
  chassis.drive_imu_reset();                  // Reset gyro position to 0
  chassis.drive_sensor_reset();               // Reset drive sensors to 0
  //chassis.odom_xyt_set(0_in, 0_in, 0_deg);    // Set the current position, you can start at a specific position with this
  chassis.drive_brake_set(MOTOR_BRAKE_HOLD);  // Set motors to hold.  This helps autonomous consistenc
  
 // ez::as::auton_selector.selected_auton_call();  // Calls selected auton from autonomous selector

//red goal rush 

lift_auto(0);
lift_wait();

Intake1.move(127);
chassis.pid_drive_set(50_in, DRIVE_SPEED, true);
// chassis.pid_wait_until(40_in);
// Intake1.move(50);
chassis.pid_wait();
//goal rush
Intake1.move(0); 
chassis.pid_turn_set(-5_deg, 100);
pros::delay(100);

arm.set_value(1);
pros::delay(50);

chassis.pid_drive_set(-23_in, DRIVE_SPEED, true);
pros::delay(800);

arm.set_value(0);
pros::delay(200);
//grab 1st ogal with mogo 
chassis.pid_turn_set(190_deg, TURN_SPEED);
pros::delay(500);

MOGO.set_value(1);
chassis.pid_drive_set(-24_in, 55, true);
pros::delay(1000);

MOGO.set_value(0);
pros::delay(200);
//grab preload 
chassis.pid_turn_set(180_deg, TURN_SPEED);
pros::delay(500);

Intake1.move(127);
chassis.pid_drive_set(37_in, DRIVE_SPEED, true);
pros::delay(1450);

Intake1.move(0);
pros::delay(200);
//grab second mogo
chassis.pid_turn_set(220_deg, TURN_SPEED);
MOGO.set_value(1);
pros::delay(500);

chassis.pid_turn_set(137_deg, TURN_SPEED);
pros::delay(500);

chassis.pid_drive_set(-50_in, DRIVE_SPEED, true);
chassis.pid_wait_until(-15_in);
chassis.pid_speed_max_set(45); 
chassis.pid_wait();

MOGO.set_value(0);
pros::delay(500);

// grab mid ring 
chassis.pid_turn_set(-45_deg, TURN_SPEED);
pros::delay(500);

chassis.pid_drive_set(22_in, DRIVE_SPEED, true);
pros::delay(750);

arm.set_value(1);
pros::delay(200);

chassis.pid_drive_set(-30_in, DRIVE_SPEED, true);
pros::delay(800);

chassis.pid_turn_set(-20_deg, TURN_SPEED);
pros::delay(500);

arm.set_value(0);
pros::delay(200);

chassis.pid_turn_set(15_deg, TURN_SPEED);
pros::delay(500);

Intake1.move(127);
chassis.pid_drive_set(10_in, DRIVE_SPEED, true);
pros::delay(800);

chassis.pid_turn_set(-45_deg, TURN_SPEED);
pros::delay(500);

chassis.pid_drive_set(10_in, DRIVE_SPEED, true);
lift_auto(1200);
lift_wait();
}

// /**
//  * Simplifies printing tracker values to the brain screen
//  */
// // void screen_print_tracker(ez::tracking_wheel *tracker, std::string name, int line) {
// //   std::string tracker_value = "", tracker_width = "";
// //   // Check if the tracker exists
// //   if (tracker != nullptr) {
// //     tracker_value = name + " tracker: " + util::to_string_with_precision(tracker->get());             // Make text for the tracker value
// //     tracker_width = "  width: " + util::to_string_with_precision(tracker->distance_to_center_get());  // Make text for the distance to center
// //   }
// //   ez::screen_print(tracker_value + tracker_width, line);  // Print final tracker text
// // }
// /**
//  * Ez screen task
//  * Adding new pages here will let you view them during user control or autonomous
//  * and will help you debug problems you're having
//  */
// // void ez_screen_task() {
// //   while (true) {
// //     // Only run this when not connected to a competition switch
// //     if (!pros::competition::is_connected()) {
// //       // Blank page for odom debugging
// //       if (chassis.odom_enabled() && !chassis.pid_tuner_enabled()) {
// //         // If we're on the first blank page...
// //         if (ez::as::page_blank_is_on(0)) {
// //           // Display X, Y, and Theta
// //           ez::screen_print("x: " + util::to_string_with_precision(chassis.odom_x_get()) +
// //                                "\ny: " + util::to_string_with_precision(chassis.odom_y_get()) +
// //                                "\na: " + util::to_string_with_precision(chassis.odom_theta_get()),
// //                            1);  // Don't override the top Page line
// //           // Display all trackers that are being used
// //           screen_print_tracker(chassis.odom_tracker_left, "l", 4);
// //           screen_print_tracker(chassis.odom_tracker_right, "r", 5);
// //           screen_print_tracker(chassis.odom_tracker_back, "b", 6);
// //           screen_print_tracker(chassis.odom_tracker_front, "f", 7);
// //         }
// //       }
// //     }
// //     // Remove all blank pages when connected to a comp switch
// //     else {
// //       if (ez::as::page_blank_amount() > 0)
// //         ez::as::page_blank_remove_all();
// //     }
// //     pros::delay(ez::util::DELAY_TIME);
// //   }
// // }
// // pros::Task ezScreenTask(ez_screen_task);
// /**
//  * Gives you some extras to run in your opcontrol:
//  * - run your autonomous routine in opcontrol by pressing DOWN and B
//  *   - to prevent this from accidentally happening at a competition, this
//  *     is only enabled when you're not connected to competition control.
//  * - gives you a GUI to change your PID values live by pressing X
//  */
// // void ez_template_extras() {
// //   // Only run this when not connected to a competition switch
// //   if (!pros::competition::is_connected()) {
// //     // PID Tuner
// //     // - after you find values that you're happy with, you'll have to set them in auton.cpp
// //     // Enable / Disable PID Tuner
// //     //  When enabled:
// //     //  * use A and Y to increment / decrement the constants
// //     //  * use the arrow keys to navigate the constants
// //     if (master.get_digital_new_press(DIGITAL_X))
// //       chassis.pid_tuner_toggle();
// //     // Trigger the selected autonomous routine
// //     if (master.get_digital(DIGITAL_B) && master.get_digital(DIGITAL_DOWN)) {
// //       pros::motor_brake_mode_e_t preference = chassis.drive_brake_get();
// //       autonomous();
// //       chassis.drive_brake_set(preference);
// //     }
// //     // Allow PID Tuner to iterate
// //     chassis.pid_tuner_iterate();
// //   }
// //   // Disable PID Tuner when connected to a comp switch
// //   else {
// //     if (chassis.pid_tuner_enabled())
// //       chassis.pid_tuner_disable();
// //   }
// // }
// /**
//  * Runs the operator control code. This function will be started in its own task
//  * with the default priority and stack size whenever the robot is enabled via
//  * the Field Management System or the VEX Competition Switch in the operator
//  * control mode.
//  *
//  * If no competition control is connected, this function will run immediately
//  * following initialize().
//  *
//  * If the robot is disabled or communications is lost, the
//  * operator control task will be stopped. Re-enabling the robot will restart the
//  * task, not resume it from where it left off.
//  */

void opcontrol() {
  
/*
tank 
mogo-B
lb- downset, l1,l2 
intake - r1 
outtake-r2 
arm - down, right  
*/

Intake1.set_brake_mode(MOTOR_BRAKE_HOLD);
  chassis.drive_brake_set(MOTOR_BRAKE_COAST);

  while (true) {
    // Gives you some extras to make EZ-Template ezier
    //ez_template_extras();

    chassis.opcontrol_tank();  // Tank control
    // chassis.opcontrol_arcade_standard(ez::SPLIT);   // Standard split arcade
    // chassis.opcontrol_arcade_standard(ez::SINGLE);  // Standard single arcade
    // chassis.opcontrol_arcade_flipped(ez::SPLIT);    // Flipped split arcade
    // chassis.opcontrol_arcade_flipped(ez::SINGLE);   // Flipped single arcade

/*arm Control*/
   if (master.get_digital(pros::E_CONTROLLER_DIGITAL_DOWN))
   {
       arm.set_value(1);
   }
   else 
   {
       arm.set_value(0);
   } 

/*Intake Control*/
if (master.get_digital(pros::E_CONTROLLER_DIGITAL_R2))
{
    Intake1.move(-127);
   // Intake2.move(127);

}
else if (master.get_digital(pros::E_CONTROLLER_DIGITAL_R1))
{
   Intake1.move(127);
  // Intake2.move(-127);
}
else 
{
  //  Intake2.move(0);
    Intake1.move(0);
}

 /*LB Control*/    
 if (master.get_digital(pros::E_CONTROLLER_DIGITAL_RIGHT))
 {
   liftPID.target_set(200);

 }
 else if (master.get_digital(pros::E_CONTROLLER_DIGITAL_L1))
 {
   liftPID.target_set(1100);
  //Intake2.move(127);
   Intake1.move(-127);
 }
 else if (master.get_digital(pros::E_CONTROLLER_DIGITAL_L2))
 {
   liftPID.target_set(0);
 }
 set_lift(liftPID.compute(LB.get_position()));

//MOGO code
if (master.get_digital(pros::E_CONTROLLER_DIGITAL_B) != lastKnownButtonBState)
{
  lastKnownButtonBState = master.get_digital(pros::E_CONTROLLER_DIGITAL_B);
  if (master.get_digital(pros::E_CONTROLLER_DIGITAL_B) && ClampState == 0 || ClampState == 2)
  {
    ClampState = 1;
  } 
  else if (master.get_digital(pros::E_CONTROLLER_DIGITAL_B) && ClampState == 1)
  {
    ClampState = 0;
  }
}  
switch (ClampState)
{
  case 0:
   MOGO.set_value(0);
  
  pros::delay(10);
  break;
  
  case 1:
    MOGO.set_value(1);
    
  pros::delay(10);
    break;
}

    pros::delay(ez::util::DELAY_TIME);  // This is used for timer calculations!  Keep this ez::util::DELAY_TIME
  }
}
