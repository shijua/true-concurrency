#!/usr/bin/env ruby

require 'json'

# test result array (for JSON output)
@testscores = []

#####################################################################

# SUPPORT FUNCTIONS:

def run_test(test_name, cmd_line, expected_image, error_as_fail=true)

  # run the picture library on the supplied command line input
  puts "> running: #{test_name}"
  puts "--------------------------------------"
  puts "run picture library on command line: #{cmd_line}"
  output = %x(./picture_lib #{cmd_line})
  test_success = $?.exitstatus == 0 
  puts output
  
  # catch tests with error codes
  if(!test_success && error_as_fail) then
    puts "  - picture library reported non-zero exit code on valid input!"
    @testscores << {"score": 0, "name": "#{test_name}", "possible": 1}
    puts ""
    return    
  end
  
  # catch tests that should report error codes
  if(test_success && !error_as_fail) then
    puts "  - picture library reported zero exit code on invalid input!"
    @testscores << {"score": 0, "name": "#{test_name}", "possible": 1}
    puts ""
    return    
  end
  
  puts ""
  
  # check final image same as expected image (if specified)
  if(expected_image) then
      
    puts "check final state of output image:"
    actual_image = cmd_line.split(" ")[1]
    system %Q(./picture_compare #{actual_image} test_images/#{expected_image} 2>&1)
    test_success = $?.exitstatus == 0
    
    # report test failure case
    if(!test_success) then
      puts "  - output picture did not match expected!"
      @testscores << {"score": 0, "name": "#{test_name}", "possible": 1}
      puts ""
      return
    end
    
    # report test success case
    puts ("  + output picture matches expected")
    @testscores << {"score": 1, "name": "#{test_name}", "possible": 1}
    puts ""
    return
  end
  
  # report no-image test success case
  puts ("  + picture library reported an error (as expected)")
  @testscores << {"score": 1, "name": "#{test_name}", "possible": 1}
  puts ""
end

#####################################################################

# MAIN PROGRAM START:

system %Q(make 2>&1)
if($?.exitstatus != 0) then
  puts "Failed to compile!"
  @testscores << {"score": 0, "name": "Compilation", "possible": 1}
else

  # basic lib-command tests (check the output for specific contents):  
  puts "----------------------------------------"
  puts "   C Picture Processing Library Tests   " 
  puts "----------------------------------------"
  puts ""  
  
  run_test("invert test 1", "test_images/test.jpg test_inverted.jpg invert", "test_inverted.jpeg")
  run_test("invert test 2", "test_images/me.jpg rave.jpg invert", "rave.jpeg")
  
  run_test("grayscale test 1", "test_images/test.jpg test_grayscale.jpg grayscale", "test_grayscale.jpeg")
  run_test("grayscale test 2", "test_images/me.jpg classic.jpg grayscale", "classic.jpeg")
  
  run_test("rotate 90 test", "test_images/test.jpg test_rotate_90.jpg rotate 90", "test_rotate_90.jpeg")
  run_test("rotate 180 test", "test_images/test.jpg test_rotate_180.jpg rotate 180", "test_rotate_180.jpeg")
  run_test("rotate 270 test", "test_images/test.jpg test_rotate_270.jpg rotate 270", "test_rotate_270.jpeg")

  run_test("flip H test 1", "test_images/test.jpg test_flip_H.jpg flip H", "test_flip_H.jpeg")
  run_test("flip H test 2", "test_images/keep_calm.jpg keep_calm_H.jpg flip H", "keep_calm_H.jpeg")
  run_test("flip V test 1", "test_images/test.jpg test_flip_V.jpg flip V", "test_flip_V.jpeg")
  run_test("flip V test 2", "test_images/keep_calm.jpg keep_calm_V.jpg flip V", "keep_calm_V.jpeg")
  
  run_test("blur test 1", "test_images/test.jpg test_blur.jpg blur", "test_blur.jpeg")
  run_test("blur test 2", "test_images/dip.jpg blip.jpg blur", "blip.jpeg")
  run_test("repeated blur test 1", "test_images/ducks2.jpg need_glasses1.jpg blur", "need_glasses1.jpeg")
  for blur_cnt in 2..10
    run_test("repeated blur test #{blur_cnt}", "need_glasses#{blur_cnt-1}.jpg need_glasses#{blur_cnt}.jpg blur", "need_glasses#{blur_cnt}.jpeg")  
  end
  
  puts "----------------------------------------"
  puts "        Parallel Blur Test Cases        " 
  puts "----------------------------------------"
  puts ""    
  
  run_test("parallel blur test 1", "test_images/test.jpg par-test_blur.jpg parallel-blur", "test_blur.jpeg")
  run_test("parallel blur test 2", "test_images/dip.jpg par-blip.jpg parallel-blur", "blip.jpeg")
  run_test("repeated parallel blur test 1", "test_images/ducks2.jpg par-need_glasses1.jpg parallel-blur", "need_glasses1.jpeg")
  for blur_cnt in 2..10
    run_test("repeated parallel blur test #{blur_cnt}", "par-need_glasses#{blur_cnt-1}.jpg par-need_glasses#{blur_cnt}.jpg parallel-blur", "need_glasses#{blur_cnt}.jpeg")  
  end
  
  puts "----------------------------------------"
  puts "           IO ERROR Test Cases          " 
  puts "----------------------------------------"
  puts ""  
  
  run_test("empty input test", "", nil, false)
  run_test("no such input test", "test_images/foo.jpg output.jpg invert", nil, false)
  
  run_test("no such process test 1", "test_images/test.jpg output.jpg invrt", nil, false)
  run_test("no such process test 2", "test_images/test.jpg output.jpg greyscale", nil, false)
  run_test("no such process test 3", "test_images/test.jpg output.jpg blar", nil, false)
  run_test("no such process test 4", "test_images/test.jpg output.jpg made-up-name", nil, false)
  
  run_test("rotate arg error test 1", "test_images/test.jpg output.jpg rotate 0", nil, false)
  run_test("rotate arg error test 2", "test_images/test.jpg output.jpg rotate 100", nil, false)
  run_test("rotate arg error test 3", "test_images/test.jpg output.jpg rotate 360", nil, false)
  
  run_test("flip arg error test", "test_images/test.jpg output.jpg flip O", nil, false)
  
  # clean up the files generated by the tests
  system %Q(make clean)
end

score = 0
total = 0
@testscores.each do |test|
  score += test[:score]
  total += test[:possible]
end

puts "Testing Score = #{score}/#{total}"



