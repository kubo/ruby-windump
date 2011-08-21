# The suffix number indicates the ruby API version.
#  18  - ruby 1.8.x
#  191 - ruby 1.9.1 and 1.9.2
#  19x - ruby 1.9.x future version which will break the API compatibility
case RUBY_VERSION
when /^1\.9/
  require 'windump_191.so'
when /^1\.8/
  require 'windump_18.so'
else
  raise 'unsupported ruby version: ' + RUBY_VERSION
end

module Windump

  @@minidump_type_map = {
    # See http://msdn.microsoft.com/en-us/library/ms680519%28v=vs.85%29.aspx
    :normal                            => 0x00000000, # MiniDumpNormal
    :with_data_segs                    => 0x00000001, # MiniDumpWithDataSegs
    :with_full_memory                  => 0x00000002, # MiniDumpWithFullMemory
    :with_handle_data                  => 0x00000004, # MiniDumpWithHandleData
    :filter_memory                     => 0x00000008, # MiniDumpFilterMemory
    :scan_memory                       => 0x00000010, # MiniDumpScanMemory
    :with_unloaded_modules             => 0x00000020, # MiniDumpWithUnloadedModules
    :with_indirectly_referenced_memory => 0x00000040, # MiniDumpWithIndirectlyReferencedMemory
    :filter_module_paths               => 0x00000080, # MiniDumpFilterModulePaths
    :with_process_thread_data          => 0x00000100, # MiniDumpWithProcessThreadData
    :with_private_read_write_memory    => 0x00000200, # MiniDumpWithPrivateReadWriteMemory
    :with_without_optional_data        => 0x00000400, # MiniDumpWithoutOptionalData
    :with_full_memory_info             => 0x00000800, # MiniDumpWithFullMemoryInfo
    :with_thread_info                  => 0x00001000, # MiniDumpWithThreadInfo
    :with_code_segs                    => 0x00002000, # MiniDumpWithCodeSegs
    :without_auxiliary_state           => 0x00004000, # MiniDumpWithoutAuxiliaryState
    :with_full_auxiliary_state         => 0x00008000, # MiniDumpWithFullAuxiliaryState
    :with_private_write_copy_memory    => 0x00010000, # MiniDumpWithPrivateWriteCopyMemory
    :ignore_inaccessible_memory        => 0x00020000, # MiniDumpIgnoreInaccessibleMemory
    :with_token_information            => 0x00040000, # MiniDumpWithTokenInformation
  }

  def self.dump_type=(val)
    self.raw_dump_type =
      if val.is_a? Array
        val.inject(0) { |type, elem| type |= @@minidump_type_map[elem] }
      else
        @@minidump_type_map[val]
      end
  end

  def self.dump_type
    @@minidump_type_map.inject([:normal]) do |ary, elem|
      ary << elem[0] if (self.raw_dump_type & elem[1]) != 0
      ary
    end
  end
end
